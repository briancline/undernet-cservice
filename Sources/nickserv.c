/* @(#)$Id: nickserv.c,v 1.7 1997/07/18 21:52:38 cvs Exp $ */

/* Undernet Channel Service (X)
 * Copyright (C) 1995-2002 Robin Thellend
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * The author can be contact by email at <csfeedback@robin.pfft.net>
 *
 * Please note that this software is unsupported and mostly
 * obsolete. It was replaced by GNUworld/CMaster. See
 * http://gnuworld.sourceforge.net/ for more information.
 */

#include "h.h"

#ifdef NICKSERV

#define NSERV_HASH 100
#define NSERV_FILE "nickserv.dat"

static struct aregnick *NServ_DB [NSERV_HASH];

static void nserv_notice(char *nick, char *msg)
{
  char buffer[1024];

  sprintf(buffer,":%s NOTICE %s :%s\r\n",NSERV_NICK,nick,msg);
  sendtoserv(buffer);
}

static int nserv_hash(char *nick)
{
  register int i,j=0;

  for(i=0; nick[i] != '\0'; i++)
  {
    j += (unsigned char) toupper(nick[i]);
  }

  return j % NSERV_HASH;
}


static struct aregnick *nserv_find(char *nick)
{
  register struct aregnick *scan;

  scan = NServ_DB [ nserv_hash(nick) ];

  while(scan != NULL && strcmp(scan->nick,nick))
    scan = scan->next;

  return scan;
}

static struct aregnick **nserv_find_ptr(char *nick)
{
  register struct aregnick **scan;

  scan = & NServ_DB [ nserv_hash(nick) ];

  while(*scan != NULL && strcmp((*scan)->nick,nick))
    scan = &(*scan)->next;

  return scan;
}


void nserv_nickserv(char *source, char *args)
{
  register aluser *luser;
  char buffer[512];
  
  if((luser=ToLuser(source))==NULL)
  {
    log("ERROR: nserv_nickserv() can't locate user!");
    return;
  }

  if(!(luser->mode&LFL_ISOPER))
  {
    notice(source,"Cette commande est réservée aux IRC Opérateurs");
    return;
  }

  if(!strcmp(args,"on"))
  {
    if(NServ_status == 1)
    {
      sprintf(buffer,"%s est déjà actif!",NSERV_NICK);
      notice(source,buffer);
    }
    else
    {
      IntroduceNickserv();
      sprintf(buffer,"%s est maintenant actif!",NSERV_NICK);
      notice(source,buffer);
    }
  }
  else if(!strcmp(args,"off"))
  {
    sprintf(buffer,"Désactivé par %s",source);
    KillNickserv(buffer);
    sprintf(buffer,"%s est maintenant désactivé!",NSERV_NICK);
    notice(source,buffer);
  }
  else
  {
    notice(source,"SYNTAXE: nickserv <on|off>");
  }
}


void nserv_addnick(char *source, char *args)
{
  register aluser *luser;
  register struct aregnick *reg;
  char buffer[512], nick[80], password[80], mask[80], email[80];
  int idx;
  
  if((luser=ToLuser(source))==NULL)
  {
    log("ERROR: nserv_addnick() can't locate user!");
    return;
  }

  if(NServ_status != 1)
  {
    notice(source,"NickServ n'est pas actif!");
    return;
  }

  if(!(luser->mode&LFL_ISOPER))
  {
    nserv_notice(source,"Cette commande est réservée aux IRC Opérateurs");
    return;
  }

  GetWord(0,args,nick);
  GetWord(1,args,mask);
  GetWord(2,args,password);
  GetWord(3,args,email);

  if(!*nick || !*mask || !*password)
  {
    nserv_notice(source,"SYNTAXE: addnick <nick> <mask> <mot_de_passe> [e-mail]");
    return;
  }

  nick[9] = '\0';
  password[19] = '\0';

  if((reg=nserv_find(nick)) != NULL)
  {
    sprintf(buffer,"Le nick \"%s\" est déjà enregistré",nick);
    nserv_notice(source,buffer);
    return;
  }

  reg = (struct aregnick *) malloc(sizeof(struct aregnick));
  memset(reg,0,sizeof(struct aregnick));

  strcpy(reg->nick, nick);
  strcpy(reg->password, password);
  strcpy(reg->email, email);
  reg->created = now;
  reg->modified = now;

  reg->mask = (struct aregmask *) malloc(sizeof(struct aregmask));
  strcpy(reg->mask->mask, mask);
  reg->mask->lastused = (time_t) 0;
  reg->mask->next = NULL;

  idx = nserv_hash(nick);
  reg->next = NServ_DB[idx];
  NServ_DB[idx] = reg;

  reg->ref = 1;

  sprintf(buffer,"Nick %s (%s) ajouté",nick,mask);
  nserv_notice(source,buffer);
}



void nserv_addmask(char *source, char *args)
{
  register aluser *luser;
  register struct aregnick *reg;
  register struct aregmask *rmask;
  char buffer[512], nick[80], mask[80];
  
  if((luser=ToLuser(source))==NULL)
  {
    log("ERROR: nserv_addnick() can't locate user!");
    return;
  }

  if(NServ_status != 1)
  {
    notice(source,"NickServ n'est pas actif!");
    return;
  }

  if(!(luser->mode&LFL_ISOPER))
  {
    nserv_notice(source,"Cette commande est réservée aux IRC Opérateurs");
    return;
  }


  GetWord(0,args,nick);
  GetWord(1,args,mask);

  if(!*nick || !*mask)
  {
    nserv_notice(source,"SYNTAXE: addmask <nick> <mask>");
    return;
  }

  nick[9] = '\0';

  if((reg=nserv_find(nick)) == NULL)
  {
    sprintf(buffer,"Le nick \"%s\" n'est pas enregistré",nick);
    nserv_notice(source,buffer);
    return;
  }

  reg->modified = now;

  rmask = (struct aregmask *) malloc(sizeof(struct aregmask));
  memset(rmask,0,sizeof(struct aregmask));

  strcpy(rmask->mask, mask);
  rmask->lastused = (time_t) 0;
  rmask->next = reg->mask;
  reg->mask = rmask;
   
  sprintf(buffer,"Mask %s ajouté a %s",mask,nick);
  nserv_notice(source,buffer);
}



void nserv_remnick(char *source, char *args)
{
  register aluser *luser;
  register struct aregnick **reg, *tmpreg;
  register struct aregmask *rmask;
  char buffer[512], nick[80];
  
  if((luser=ToLuser(source))==NULL)
  {
    log("ERROR: nserv_addnick() can't locate user!");
    return;
  }

  if(NServ_status != 1)
  {
    notice(source,"NickServ n'est pas actif!");
    return;
  }

  if(!(luser->mode&LFL_ISOPER))
  {
    nserv_notice(source,"Cette commande est réservée aux IRC Opérateurs");
    return;
  }


  GetWord(0,args,nick);

  if(!*nick)
  {
    nserv_notice(source,"SYNTAXE: remnick <nick>");
    return;
  }

  nick[9] = '\0';

  if( *(reg=nserv_find_ptr(nick)) == NULL)
  {
    sprintf(buffer,"Le nick \"%s\" n'est pas enregistré",nick);
    nserv_notice(source,buffer);
    return;
  }

  tmpreg = *reg;
  *reg = tmpreg->next;

  tmpreg->next = NULL;
  tmpreg->ref--;

  if(tmpreg->ref == 0)
  {
    while((rmask=tmpreg->mask) != NULL)
    {
      tmpreg->mask = rmask->next;
      free(rmask);
    }
    free(tmpreg);  
  }

  sprintf(buffer,"Nick %s effacé",nick);
  nserv_notice(source,buffer);
}


void nserv_remmask(char *source, char *args)
{
  register aluser *luser;
  register struct aregnick *reg;
  register struct aregmask **rmask, *tmprmask;
  char buffer[512], nick[80], mask[80];
  
  if((luser=ToLuser(source))==NULL)
  {
    log("ERROR: nserv_addnick() can't locate user!");
    return;
  }

  if(NServ_status != 1)
  {
    notice(source,"NickServ n'est pas actif!");
    return;
  }

  if(!(luser->mode&LFL_ISOPER))
  {
    nserv_notice(source,"Cette commande est réservée aux IRC Opérateurs");
    return;
  }


  GetWord(0,args,nick);
  GetWord(1,args,mask);

  if(!*nick || !*mask)
  {
    nserv_notice(source,"SYNTAXE: remmask <nick> <mask>");
    return;
  }

  nick[9] = '\0';

  if((reg=nserv_find(nick)) == NULL)
  {
    sprintf(buffer,"Le nick \"%s\" n'est pas enregistré",nick);
    nserv_notice(source,buffer);
    return;
  }

  reg->modified = now;

  rmask = & reg->mask;

  while(*rmask != NULL && strcmp((*rmask)->mask, mask))
    rmask = &(*rmask)->next;

  if(*rmask == NULL)
  {
    sprintf(buffer,"%s n'as pas de mask %s",nick,mask);
    nserv_notice(source,buffer);
    return;
  }

  tmprmask = *rmask;
  *rmask = tmprmask->next;

  free(tmprmask);
   
  sprintf(buffer,"Mask %s enlevé de %s",mask,nick);
  nserv_notice(source,buffer);
}



void nserv_nickinfo(char *source, char *args)
{
  register aluser *luser;
  register struct aregnick *reg;
  register struct aregmask *rmask;
  char buffer[512], nick[80];
  
  if((luser=ToLuser(source))==NULL)
  {
    log("ERROR: nserv_addnick() can't locate user!");
    return;
  }

  if(NServ_status != 1)
  {
    notice(source,"NickServ n'est pas actif!");
    return;
  }

  GetWord(0,args,nick);

  if(!*nick)
  {
    nserv_notice(source,"SYNTAXE: nickinfo <nick>");
    return;
  }

  nick[9] = '\0';

  if((reg=nserv_find(nick)) == NULL)
  {
    sprintf(buffer,"Le nick \"%s\" n'est pas enregistré",nick);
    nserv_notice(source,buffer);
    return;
  }

  sprintf(buffer,"Nick: %s",reg->nick);
  nserv_notice(source,buffer);
  sprintf(buffer,"Création: %.24s",ctime(&reg->created));
  nserv_notice(source,buffer);
  sprintf(buffer,"Modification: %.24s",ctime(&reg->modified));
  nserv_notice(source,buffer);

  if((!strcmp(source,reg->nick) && luser->regnick && luser->reglimit == 0)
      || (luser->mode&LFL_ISOPER))
  {
    sprintf(buffer,"Mot de passe: %s",reg->password);
    nserv_notice(source,buffer);

    sprintf(buffer,"E-mail: %s",reg->email);
    nserv_notice(source,buffer);
  }

  for(rmask = reg->mask; rmask != NULL; rmask = rmask->next)
  {
    if(rmask->lastused == (time_t) 0)
      sprintf(buffer,"Mask: %s  (jamais utilisé)",rmask->mask);
    else
      sprintf(buffer,"Mask: %s  %.24s",rmask->mask, ctime(&rmask->lastused));
    nserv_notice(source,buffer);
  }
}


void nserv_identify(char *source, char *args)
{
  register aluser *luser;
  register struct aregmask *rmask;
  char buffer[512], password[80];
  
  if((luser=ToLuser(source))==NULL)
  {
    log("ERROR: nserv_identify() can't locate user!");
    return;
  }

  if(NServ_status != 1)
  {
    notice(source,"NickServ n'est pas actif!");
    return;
  }

  GetWord(0,args,password);

  if(!*password)
  {
    nserv_notice(source,"SYNTAXE: identify <mot_de_passe>");
    return;
  }

  if(luser->regnick == NULL)
  {
    sprintf(buffer,"Le nick \"%s\" n'est pas enregistré",source);
    nserv_notice(source,buffer);
    return;
  }

  if(luser->reglimit == 0)
  {
    nserv_notice(source,"Vous vous êtes déjà identifié");
    return;
  }

  sprintf(buffer,"%s!%s@%s",luser->nick,luser->username,luser->site);

  rmask = luser->regnick->mask;
  while(rmask != NULL && !match(buffer,rmask->mask))
    rmask = rmask->next;

  if(rmask && !strcmp(password,luser->regnick->password))
  {
    luser->reglimit = 0;
    rmask->lastused = now;
    nserv_notice(source,"Mot de passe accepté. Bienvenue!");
  }
  else
  {
    nserv_notice(source,"Le mot de passe est \002incorrect!\002");
  }
}


void nserv_ghost(char *source, char *args)
{
  register aluser *luser, *luser2;
  register struct aregnick *reg;
  register struct aregmask *rmask;
  char buffer[512], nick[80], password[80];
  
  if((luser=ToLuser(source))==NULL)
  {
    log("ERROR: nserv_ghost() can't locate user!");
    return;
  }

  if(NServ_status != 1)
  {
    notice(source,"NickServ n'est pas actif!");
    return;
  }

  GetWord(0,args,nick);
  GetWord(1,args,password);

  if(!*nick || !*password)
  {
    nserv_notice(source,"SYNTAXE: ghost <nick> <mot_de_passe>");
    return;
  }

  if(!strcmp(nick,source))
  {
    nserv_notice(source,"Ah oui? Vous n'avez pas l'air d'un ghost!");
    return;
  }

  luser2 = ToLuser(nick);

  if(luser2 == NULL)
  {
    sprintf(buffer,"Le nick \"%s\" n'est pas utilisé présentement",nick);
    nserv_notice(source,buffer);
    return;
  }

  reg = nserv_find(nick);

  if(reg == NULL)
  {
    sprintf(buffer,"Le nick \"%s\" n'est pas enregistré",nick);
    nserv_notice(source,buffer);
    return;
  }

  sprintf(buffer,"%s!%s@%s",luser->nick,luser->username,luser->site);

  rmask = reg->mask;
  while(rmask != NULL && !match(buffer,rmask->mask))
    rmask = rmask->next;

  if(rmask && !strcmp(password,luser->regnick->password))
  {
    sprintf(buffer, ":%s KILL %s :%s (Ghost kill demandé par %s)\r\n",
            NSERV_NICK, nick, NSERV_NICK, source);
    sendtoserv(buffer);
    onquit(nick);

    sprintf(buffer,"Le ghost \"%s\" a été killé",nick);
    nserv_notice(source,buffer);
  }
  else
  {
    nserv_notice(source,"Le mot de passe est \002incorrect!\002");
  }
}


void nserv_nicknewpass(char *source, char *args)
{
  register aluser *luser;
  register struct aregnick *reg;
  char buffer[512], nick[80], password[80];
  
  if((luser=ToLuser(source))==NULL)
  {
    log("ERROR: nserv_nicknewpass() can't locate user!");
    return;
  }

  if(NServ_status != 1)
  {
    notice(source,"NickServ n'est pas actif!");
    return;
  }

  GetWord(0,args,nick);
  GetWord(1,args,password);

  if(!*nick && !*password)
  {
    nserv_notice(source,"SYNTAXE: nicknewpass [nick] <mot_de_passe>");
    return;
  }

  if(!*password)
  {
    strcpy(password,nick);
    strcpy(nick,source);
  }

  if(strcmp(nick,source) && !(luser->mode&LFL_ISOPER))
  {
    nserv_notice(source,"Vous n'avez pas le droit de changer le mot de passe de quelqu'un d'autre");
    return;
  }

  if(!(luser->mode&LFL_ISOPER) && (luser->regnick == NULL ||
     luser->reglimit != 0))
  {
    nserv_notice(source,"Vous devez d'abord vous identifier");
    return;
  }

  if((luser->mode&LFL_ISOPER))
  {
    reg = nserv_find(nick);
  }
  else
  {
    reg = luser->regnick;
  }

  if(reg == NULL)
  {
    sprintf(buffer,"Le nick \"%s\" n'est pas enregistré",nick);
    nserv_notice(source,buffer);
    return;
  }

  password[19] = '\0';

  strcpy(reg->password,password);

  sprintf(buffer,"Le nouveau mot de passe pour %s est %s",
          reg->nick,reg->password);
  nserv_notice(source,buffer);
}


void nserv_nicknewemail(char *source, char *args)
{
  register aluser *luser;
  register struct aregnick *reg;
  char buffer[512], nick[80], email[80];
  
  if((luser=ToLuser(source))==NULL)
  {
    log("ERROR: nserv_nicknewemail() can't locate user!");
    return;
  }

  if(NServ_status != 1)
  {
    notice(source,"NickServ n'est pas actif!");
    return;
  }

  GetWord(0,args,nick);
  GetWord(1,args,email);

  if(!*nick || !*email)
  {
    nserv_notice(source,"SYNTAXE: nicknewemail <nick> <e-mail>");
    return;
  }

  if(!(luser->mode&LFL_ISOPER))
  {
    nserv_notice(source,"Cette commande est réservée aux IRC Opérateurs");
    return;
  }

  reg = nserv_find(nick);
  if(reg == NULL)
  {
    sprintf(buffer,"Le nick \"%s\" n'est pas enregistré",nick);
    nserv_notice(source,buffer);
    return;
  }

  strcpy(reg->email,email);

  sprintf(buffer,"Le nouveau e-mail pour %s est %s",reg->nick,reg->email);
  nserv_notice(source,buffer);
}


void nserv_save(void)
{
  register struct aregnick *reg;
  register struct aregmask *mask;
  register int i;
  FILE *fp;

  fp = fopen(NSERV_FILE".new","w");
  if(fp == NULL)
    return;

  for(i=0; i<NSERV_HASH; i++)
  {
    for(reg=NServ_DB[i]; reg != NULL; reg=reg->next)
    {
      fwrite(reg, sizeof(struct aregnick), 1, fp);

      for(mask=reg->mask; mask != NULL; mask=mask->next)
        fwrite(mask, sizeof(struct aregmask), 1, fp);
    }
  }

  fclose(fp);

  rename(NSERV_FILE".new", NSERV_FILE);
}


void nserv_load(void)
{
  struct aregnick reg;
  struct aregmask mask;
  register struct aregnick *treg;
  register struct aregmask *tmask;
  FILE *fp;

  fp = fopen(NSERV_FILE,"r");
  if(fp == NULL)
    return;

  while(fread(&reg, sizeof(struct aregnick), 1, fp) != 0)
  {
    treg = (struct aregnick *) malloc (sizeof(struct aregnick));
    memcpy(treg, &reg, sizeof(struct aregnick));
    treg->next = NULL;
    if(treg->mask != NULL)
    {
      treg->mask = NULL;
      while(fread(&mask, sizeof(struct aregmask), 1, fp) != 0)
      {
        int end = 0;
        tmask = (struct aregmask *) malloc (sizeof(struct aregmask));
        memcpy(tmask, &mask, sizeof(struct aregmask));
        if(tmask->next == NULL)
          end = 1;
        tmask->next = treg->mask;
        treg->mask = tmask;
        if(end)
          break;
      }
    }
    treg->next = NServ_DB[nserv_hash(treg->nick)];
    NServ_DB[nserv_hash(treg->nick)] = treg;
  }

  fclose(fp);
}

void nserv_checkregnick(char *nick)
{
  register aluser *luser;
  char buffer[512];

  if(NServ_status != 1)
    return;

  luser = ToLuser(nick);
  if(luser == NULL || luser->regnick == NULL || luser->reglimit == 0 ||
     luser->reglimit > now)
    return;

  /* timeout for receiving valid password has expired */
  nserv_notice(nick,"Vous ne vous êtes pas identifié. Vous devez partir!");
  /*sprintf(buffer,":%s KILL %s :NickServ (Non vérifié)\r\n",mynick,nick);
  sendtoserv(buffer);*/
  sprintf(buffer,
          ":%s GLINE + +%s@%s %d :Échec de l'identification pour le nick %s. "
          "Accès non autorisé au serveur pendant %d secondes.\r\n",
          mynick,luser->username,luser->site,NSERV_GLINETIME,nick,
          NSERV_GLINETIME);
    sendtoserv(buffer);
  onquit(nick);
}

void nserv_quit(aluser *user)
{
  if(user->regnick)
  {
    user->regnick->ref--;

    if(user->regnick->ref == 0)
    {
      register struct aregmask *rmask;
      while((rmask=user->regnick->mask) != NULL)
      {
        user->regnick->mask = rmask->next;
        free(rmask);
      }
      free(user->regnick);
    }

    user->regnick = NULL;
  }
}

void nserv_nick(char *newnick, aluser *user)
{
  char buffer[200];

  nserv_quit(user);

  if(NServ_status != 1)
  {
    return;
  }

  user->regnick = nserv_find(newnick);

  if(user->regnick)
  {
    user->regnick->ref++;
    user->reglimit = now + NSERV_DELAY;
    sprintf(buffer,"Ce nick est enregistré. Vous avez 60 secondes pour vous identifier avec la commande \"/msg %s identify mot_de_passe\"",NSERV_NICK);
    nserv_notice(newnick,buffer);
    AddEvent(EVENT_CHECKREGNICK,user->reglimit,newnick);
  }
}


void nserv_onop(char *channel, auser *user)
{
  if(user == NULL) /* NULL if X is opped */
    return;

  if(NServ_status != 1)
    return;

  if(user->N->regnick == NULL)
  {
    nserv_notice(user->N->nick,"Vous ne pouvez pas être op sans être enregistré");
    changemode(channel,"-o",user->N->nick,1);
    user->chanop = 0;
  }
}

#endif /* NICKSERV */
