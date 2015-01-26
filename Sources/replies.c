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


/* Very appreciated contribution from:  (just add your name here)
   Stephan Mantler aka Step <smantler@atpibm6000.tuwien.ac.at>
   Carlo Kid aka Run <carlo@tnrunaway.tn.tudelft.nl>
   nirvana <nirvana@nil.fut.es>
 */

#include "h.h"

alang Lang[NO_LANG] =
{
  {L_ENGLISH, "en", "english"},
  {L_DUTCH, "nl", "dutch"},
  {L_FRENCH, "fr", "français"},
  {L_SPANISH, "es", "español"},
  {L_GERMAN, "de", "deutsch"}
};

/* the %s will be expanded to the channel name
   notaccessreply is only english for now */
char *notaccessreply[] =
{
  "%s: Nice try. But I'm not a fool!",
  "%s: Cheating is very bad you know!",
  "%s: Do I know you?",
  "You should come on %s more often",
  "Ha! Ha! I don't think people on %s would like that!",
  "%s: Pfft! Nice try!",
  "%s: No, I don't think so.. you're not my type :P",
  "go read the %s manual.. then I might consider it",
  "I would.. but.. one teensy problem.  You're not in %s's database",
  "Oh.. are you trying to communicate with me?",
  "You see, %s is some kinda private thing...",
  NULL
};

void ReplyNotAccess(char *nick, char *channel)
{
  static int index = 0;
  register int i;
  char buffer[200];

  for (i = rand() % 5 + 1; i > 0; i--)
    if (notaccessreply[++index] == NULL)
      index = 0;

  sprintf(buffer, notaccessreply[index], channel);
  notice(nick, buffer);
}

void RandomChannel(char *source)
{
  register int hash, offset;
  register achannel *chan;
  char buffer[200];

  hash = rand() % 1000;
  offset = rand() % 100;

  chan = ChannelList[hash];
  while (chan == NULL)
  {
    chan = ChannelList[(++hash) % 1000];
    if (hash > 2000)
      return;
  }
  while (offset > 0 && !chan->on)
  {
    chan = chan->next;
    if (chan == NULL)
    {
      while (chan == NULL)
      {
	chan = ChannelList[(++hash) % 1000];
	if (hash > 2000)
	  return;
      }
    }
    offset--;
  }
  sprintf(buffer, "%s is cool!", chan->name);
  notice(source, buffer);
}

void Say(char *source, char *args)
{
  char buffer[1024], target[80], global[] = "*";

  if (Access(global, source) < 900)
  {
    return;	/*silently */
  }

  GetWord(0, args, target);
  args = ToWord(1, args);

  if (!*target || !*args)
  {
    notice(source, "Syntax: say [#channel] [whatever]");
    return;
  }

  sprintf(buffer, ":%s PRIVMSG %s :%s\n", mynick, target, args);
  sendtoserv(buffer);
}


void ServNotice(char *source, char *args)
{
  char buffer[1024], target[80], global[] = "*";

  if (Access(global, source) < 600)
  {
    return;	/*silently */
  }

  GetWord(0, args, target);
  args = ToWord(1, args);

  if (!*target || !*args)
  {
    notice(source, "SYNTAX: servnotice [#channel] [whatever]");
    return;
  }

  sprintf(buffer, "[Channel Service: %s] %s", source, args);
  servnotice(target, buffer);
}


char *replies[][NO_LANG] =
{
/* RPL_NOTONCHANNEL */
  {"I am not on that channel!",	/* english */
    "Ik zit niet op dat kanaal!",	/* dutch */
    "Je ne suis pas sur ce canal!",	/* french */
    "No estoy en ese canal!",	/* spanish */
    "Ich bin nicht auf diesem Kanal!"	/* german */
  },

/* RPL_NOTCHANOP */
  {"I am not channel operator",	/* english */
    "Ik ben geen channel operator",	/* dutch */
    "Je ne suis pas opérateur de canal",	/* french */
    "No soy operador del canal",	/* spanish */
    "Ich bin kein Betreiber auf diesem Kanal"	/* german */
  },

/* RPL_OPONLY */
  {"This channel is in OpOnly Mode",	/* english */
    "Dit kanaal is in OpOnly Mode",	/* dutch */
    "Ce canal est en mode OpOnly",	/* french */
    "Ese canal está en modo OpOnly",	/* spanish */
    "Dieser Kanal is im OpOnly-Modus"	/* german */
  },

/* RPL_OPSELFONLY */
  {"You can only op yourself in OpOnly mode",	/* english */
    "In OpOnly mode mag je alleen jezelf operator maken",	/* dutch */
    "Vous ne pouvez opper que vous-même en mode OpOnly",	/* french */
    "Solo puedes ser operador en modo OpOnly",	/* spanish */
    "Sie koennen nur sich selbst zum Betreiber machen (Kanal im OpOnly-Modus)"	/* german */
  },

/* RPL_NOSUCHNICK */
  {"No such nickname",		/* english */
    "Deze NICK is niet in gebruik",	/* dutch */
    "Ce nick n'existe pas",	/* french */
    "Ese nick no existe",	/* spanish */
    "Diesen Nickname gibt es nicht"	/* german */
  },

/* RPL_ALREADYONCHANNEL */
  {"This user is already on channel",	/* english */
    "Deze persoon is al op dat kanaal",		/* dutch */
    "Cet usager est déjà sur le canal",		/* french */
    "Ese usuario ya está en el canal",	/* spanish */
    "Dieser Benutzer ist bereits auf dem Kanal"		/* german */
  },

/* RPL_IINVITED */
  {"I invited %s to %s",	/* english */
    "%s is uitgenodigd naar %s te komen",	/* dutch */
    "J'ai invité %s sur %s",	/* french */
    "Yo he invitado a %s en %s",	/* spanish */
    "Ich haben %s eingeladen, auf %s zu kommen"		/* german */
  },

/* RPL_YOUAREINVITED */
  {"%s invites you to %s",	/* english */
    "%s vraagt of naar %s komt",	/* dutch */
    "%s vous invite sur %s",	/* french */
    "%s te invita en %s",	/* spanish */
    "%s laedt Sie ein, auf %s zu kommen"	/* german */
  },

/* RPL_ALWAYSOPWASACTIVE */
  {"AlwaysOp was active! It's now deactivated",		/* english */
    "AlwaysOp was actief! Het is nu uitgeschakeld",	/* dutch */
    "AlwaysOp était actif! Il est maintenant désactivé",	/* french */
    "AlwaysOp estaba activado! Ahora ya esta desactivado",	/* spanish */
    "AlwaysOp war aktiv! Es wurde ausgeschalten"	/* german */
  },

/* RPL_ALWAYSOP */
  {"This channel is in AlwaysOp mode!",		/* english */
    "Dit kanaal is in AlwaysOp mode!",	/* dutch */
    "Ce canal est en mode AlwaysOp!",	/* french */
    "Este canal está en modo AlwaysOp!",	/* spanish */
    "Dieser Kanal is im AlwaysOp-Modus!"	/* german */
  },

/* RPL_KICK1ST */
  {"You are not allowed to KICK me!",	/* english */
    "Het is niet toegestaan mij te KICKen!",	/* dutch */
    "Vous n'avez pas le droit de me kicker!",	/* french */
    "No estas autorizado para kickearme!",	/* spanish */
    "Sie duerfen mich nicht kicken!"	/* german */
  },

/* RPL_KICK2ND */
  {"Please STOP kicking me!",	/* english */
    "STOP mij te KICKen!",	/* dutch */
    "Veuillez arrêter de me kicker!",	/* french */
    "Por favor ¡para de kickearme!",	/* spanish */
    "Bitte hoeren Sie auf, mich zu kicken!"	/* german */
  },

/* RPL_CHANNOTEXIST */
  {"That channel does not exist!",	/* english */
    "Dat kanaal bestaat niet!",	/* dutch */
    "Ce canal n'existe pas!",	/* french */
    "Ese canal no existe!",	/* spanish */
    "Diesen Kanal gibt es nicht!"	/* german */
  },

/* RPL_BADFLOODLIMIT */
  {"value of FLOODPRO must be in the range [3-20] or 0 to turn it off",
    "FLOODPRO moet een waarde hebben tussen 3 en 20 of 0 om het uit te schakelen",
    "la valeur de FLOODPRO doit être entre 3 et 20 ou 0 pour le désactiver",
    "El valor de FLOODPRO tiene que estar entre 3 y 20 ó 0 para desactivarlo",
    "der Wert fon FLOODPRO muss im Bereich von 3 und 20 sein (oder 0 um es abzuschalten)"
  },

/* RPL_SETFLOODLIMIT */
  {"value of FLOODPRO is now %d",	/* english */
    "De waarde van FLOODPRO is nu %d",	/* dutch */
    "la valeur de FLOODPRO est maintenant %d",	/* french */
    "El valor de FLOOPRO está ahora %d",	/* spanish */
    "FLOODPRO hat jetzt den Wert %d"	/* german */
  },

/* RPL_BADNICKFLOODLIMIT */
  {"value of NICKFLOODPRO must be in the range [3-10] or 0 to turn it off",
    "NICKFLOODPRO moet een waarde hebben tussen 3 en 10 of 0 om het uit te schakelen",
    "la valeur de NICKFLOODPRO doit être entre 3 et 10 ou 0 pour le désactiver",
    "El valor de NICKFLOOPRO tiene que estar entre 3 y 10 ó 0 para desactivarlo",
    "Werte fuer NICKFLOODPRO muessen im Bereich von 3 bis 10 sein. Oder 0 um es abzuschalten"
  },

/* RPL_SETNICKFLOODLIMIT */
  {"value of NICKFLOODPRO is now %d",	/* english */
    "De waarde van NICKFLOODPRO is nu %d",	/* dutch */
    "la valeur de NICKFLOODPRO est maintenant %d",	/* french */
    "El valor de NICKFLOOPRO está ahora en %d",		/* spanish */
    "der Wert fuer NICKFLOODPRO ist jetzt %d"	/* german */
  },

/* RPL_BADMASSDEOPLIMIT */
  {"value of MASSDEOPPRO must be in the range [3-10] or 0 to turn it off",
    "MASSDEOPPRO moet een waarde hebben tussen 3 en 10 of 0 om het uit te schakelen",
    "la valeur de MASSDEOPPRO doit être entre 3 et 10 ou 0 pour le désactiver",
    "El valor de MASSDEOPPRO tiene que estar entre 3 y 10 ó 0 para desactivarlo",
    "der Wert von MASSDEOPPRO muss zwischen 3 und 10 liegen, oder 0 um es abzuschalten"
  },

/* RPL_SETMASSDEOPLIMIT */
  {"value of MASSDEOPPRO is now %d",	/* english */
    "De waarde van MASSDEOPPRO is nu %d",	/* dutch */
    "la valeur de MASSDEOPPRO est maintenant %d",	/* french */
    "El valor de MASSDEOPPRO está ahora en %d",		/* spanish */
    "MASSDEOPPRO ist jetzt %d"	/* german */
  },

/* RPL_NOOPON */
  {"value of NOOP is now ON",	/* english */
    "NOOP mode is nu ingeschakeld",	/* dutch */
    "la valeur de NOOP est maintenant ON",	/* french */
    "El valor de NOOP está ahora en ON",	/* spanish */
    "NOOP hat jetzt den Wert EIN"	/* german */
  },

/* RPL_NOOPOFF */
  {"value of NOOP is now OFF",	/* english */
    "NOOP mode is nu uit",	/* dutch */
    "la valeur de NOOP est maintenant OFF",	/* french */
    "El valor de NOOP está ahora en OFF",	/* spanish */
    "NOOP hat jetzt den Wert AUS"	/* german */
  },

/* RPL_BADNOOP */
  {"value of NOOP must be ON or OFF",	/* english */
    "Kies 'ON' of 'OFF' voor de waarde van NOOP",	/* dutch */
    "la valeur de NOOP doit être ON ou OFF",	/* french */
    "El valor de NOOP debe ser ON o OFF",	/* spanish */
    "NOOP kann nur EIN oder AUS sein"	/* german */
  },

/* RPL_ALWAYSOPON */
  {"value of ALWAYSOP is now ON",	/* english */
    "ALWAYSOP mode is nu ingeschakeld",		/* dutch */
    "la valeur de ALWAYSOP est maintenant ON",	/* french */
    "El valor de ALWAYSOP es ahora ON",		/* spanish */
    "ALWAYSOP ist jetzt EIN"	/* german */
  },

/* RPL_ALWAYSOPOFF */
  {"value of ALWAYSOP is now OFF",	/* english */
    "ALWAYSOP mode is nu uit",	/* dutch */
    "la valeur de ALWAYSOP est maintenant OFF",		/* french */
    "El valor de ALWAYSOP es ahora OFF",	/* spanish */
    "ALWAYSOP ist jetzt AUS"	/* german */
  },

/* RPL_BADALWAYSOP */
  {"value of ALWAYSOP must be ON or OFF",	/* english */
    "Kies 'ON' of 'OFF' voor de waarde van ALWAYSOP",	/* dutch */
    "la valeur de ALWAYSOP doit être ON ou OFF",	/* french */
    "El valor de ALWAYSOP debe ser ON o OFF",	/* spanish */
    "Werte fuer ALWAYSOP sind EIN oder AUS"	/* german */
  },

/* RPL_OPONLYON */
  {"value of OPONLY is now ON",	/* english */
    "OPONLY mode is nu ingeschakeld",	/* dutch */
    "la valeur de OPONLY est maintenant ON",	/* french */
    "El valor de OPONLY es ahora ON",	/* spanish */
    "der Wert fuer OPONLY ist jetzt EIN"	/* german */
  },

/* RPL_OPONLYOFF */
  {"value of OPONLY is now OFF",	/* english */
    "OPONLY mode is nu uit",	/* dutch */
    "la valeur de OPONLY est maintenant OFF",	/* french */
    "El valor de OPONLY es ahora OFF",	/* spanish */
    "der Wert fuer OPONLY ist jetzt AUS"	/* german */
  },

/* RPL_BADOPONLY */
  {"value of OPONLY must be ON or OFF",		/* english */
    "Kies 'ON' of 'OFF' voor de waarde van OPONLY",	/* dutch */
    "la valeur de OPONLY doit être ON ou OFF",	/* french */
    "El valor de OPONLY debe ser ON o OFF",	/* spanish */
    "Werte fuer OPONLY sind EIN oder AUS"	/* german */
  },

/* RPL_AUTOTOPICON */
  {"value of AUTOTOPIC is now ON",	/* english */
    "AUTOTOPIC mode is nu ingeschakeld",	/* dutch */
    "la valeur de AUTOTOPIC est maintenant ON",		/* french */
    "El valor de AUTOTOPIC es ahora ON",	/* spanish */
    "der Wert fuer AUTOTOPIC ist jetzt EIN"	/* german */
  },

/* RPL_AUTOTOPICOFF */
  {"value of AUTOTOPIC is now OFF",	/* english */
    "AUTOTOPIC mode is nu uit",	/* dutch */
    "la valeur de AUTOTOPIC est maintenant OFF",	/* french */
    "El valor de AUTOTOPIC es ahora OFF",	/* spanish */
    "der Wert fuer AUTOTOPIC ist jetzt AUS"	/* german */
  },

/* RPL_BADAUTOTOPIC */
  {"value of AUTOTOPIC must be ON or OFF",	/* english */
    "Kies 'ON' of 'OFF' voor de waarde van AUTOTOPIC",	/* dutch */
    "la valeur de AUTOTOPIC doit être ON ou OFF",	/* french */
    "El valor de AUTOTOPIC debe ser ON o OFF",	/* spanish */
    "Werte fuer AUTOTOPIC sind EIN oder AUS"	/* german */
  },

/* RPL_STRICTOPON */
  {"value of STRICTOP is now ON",	/* english */
    "value of STRICTOP is now ON",	/* dutch */
    "value of STRICTOP is now ON",	/* french */
    "El valor de STRICTOP es ahora ON",		/* spanish */
    "value of STRICTOP is now ON"	/* german */
  },

/* RPL_STRICTOPOFF */
  {"value of STRICTOP is now OFF",	/* english */
    "value of STRICTOP is now OFF",	/* dutch */
    "value of STRICTOP is now OFF",	/* french */
    "El valor de STRICTOP es ahora OFF",	/* spanish */
    "value of STRICTOP is now OFF"	/* german */
  },

/* RPL_BADSTRICTOP */
  {"value of STRICTOP must be ON or OFF",	/* english */
    "value of STRICTOP must be ON or OFF",	/* dutch */
    "value of STRICTOP must be ON or OFF",	/* french */
    "El valor de STRICTOP debe ser ON o OFF",	/* spanish */
    "value of STRICTOP must be ON or OFF"	/* german */
  },

/* RPL_BADUSERFLAGS */
  {"New value can be 1-AUTOOP or 0-NO AUTOOP",
    "De nieuwe waarde kan zijn 1-AUTOOP, 2-PROTECT ou 3-BOTH",
    "La nouvelle valeur peut être 1-AUTOOP ou 0-PAS DE AUTOOP",
    "El nuevo valor puede ser 1-AUTOOP o 0- NOAUTOOP",	/* spanish */
    "Der neue Wert kann 1-AUTOOP, 2-PROTECT oder 3-BEIDE sein."
  },

/* RPL_SETUSERFLAGS */
  {"value of USERFLAGS is now %d",	/* english */
    "de waarde van USERFLAGS is nu %d",		/* dutch */
    "la nouvelles valeur de USERFLAGS est %d",	/* french */
    "El nuevo valor de USERFLAGS es ahora  %d",		/* spanish */
    "USERFLAGS ist jetzt %d"	/* german */
  },

/* RPL_KNOWNLANG */
  {"Known languages are",	/* english */
    "De volgende talen zijn bekend",	/* dutch */
    "Je connais les langues",	/* french */
    "Yo hablo y escribo los siguientes idiomas",	/* spanish */
    "Folgende Sprachen sind bekannt"	/* german */
  },

/* RPL_SETLANG */
  {"Default language is now %s (%s)",	/* english */
    "De default taal is nu %s (%s)",	/* dutch */
    "La langue par défaut est maintenant %s (%s)",	/* french */
    "El idioma configurado por defecto es ahora %s (%s)",	/* spanish */
    "Die neue Default-Sprache ist %s (%s)"	/* german */
  },

/* RPL_STATUS1 */
  {"Channel %s has %d user%s (%d operator%s)",	/* english */
    "Kanaal %s heeft %d deelnemer%s (%d operator%s)",	/* dutch */
    "Canal %s a %d usager%s (%d opérateur%s)",	/* french */
    "El canal %s tiene %d usuarios%s (%d operadores%s)",	/* spanish */
    "Kanal %s hat %d Benutzer (%d Betreiber)"	/* german */
/* ### NOTE the german version does not use plurals! 
   The code reflects this. */
  },

/* RPL_STATUS2 */
  {"Mode is +%s",		/* english */
    "Mode is +%s",		/* dutch */
    "Le mode est +%s",		/* french */
    "El modo es +%s",		/* spanish */
    "Der Modus ist +%s"		/* german */
  },

/* RPL_STATUS3 */
  {"Default user flags%s",	/* english */
    "Default 'user flags'%s",	/* dutch */
    "Flags d'usager par défaut%s",	/* french */
    "Flags por defectos son %s",	/* spanish */
    "Default-Flags fuer Benutzer %s"	/* german */
  },

/* RPL_STATUS4 */
  {"Default language is %s",	/* english */
    "Default taal is %s",	/* dutch */
    "Langue par défaut %s",	/* french */
    "Idioma por defecto %s",	/* spanish */
    "Default-Sprache ist %s"	/* german */
  },

/* RPL_STATUS5 */
  {"Channel has been idle for %d second%s",	/* english */
    "Het kanaal is %d second%s idle",	/* dutch */
    "Le canal est inactif depuis %d seconde%s",		/* french */
    "El canal no está activo desde hace %d segundos%s",		/* spanish */
    "Der Kanal ist seit %d Sekunden inaktiv."	/* german */
  },

/* RPL_SETCHANDEFS */
  {"Channel defaults set.",	/* english */
    "Kanaal defaults geregistreerd.",	/* dutch */
    "L'état du canal est enregistré",	/* french */
    "Los nuevos cambios han sido registrados",	/* spanish */
    "Default-Werte fuer den Kanal wurden gesetzt"	/* german */
  },

/* RPL_NOTDEF */
  {"That channel is not in my default channel list",	/* english */
    "Dat kanaal staat niet in mijn default kanaal lijst",	/* dutch */
    "Ce canal n'est pas sur ma liste",	/* french */
    "Ese canal no está en mi lista por defecto",	/* spanish */
    "Dieser Kanal ist nicht auf meiner Liste"	/* german */
  },

/* RPL_REMDEF */
  {"Channel removed from default channel list",		/* english */
    "Het kanaal is van de default kanaal lijst verwijderd",	/* dutch */
    "Ce canal n'est plus sur ma liste",		/* french */
    "Canal eliminado de la lista por defecto",	/* spanish */
    "Der Kanal wurde aus meiner Liste entfernt"		/* german */
  },

/* RPL_NOMATCH */
  {"No match.",			/* english */
    "Niet gevonden.",		/* dutch */
    "Introuvable",		/* french */
    "No se encuentra",		/* spanish */
    "Nicht gefunden."		/* german */
  },

/* RPL_NOOP */
  {"Sorry. This channel is in NoOp mode!",	/* english */
    "Sorry. Dit kanaal is in NoOp mode!",	/* dutch */
    "Désolé. Ce canal est en mode NoOp!",	/* french */
    "Lo siento, ¡Ese canal está en modo NoOp!",		/*  spanish */
    "Tut mir leid, aber dieser Kanal ist im NoOp-Modus!"	/* german */
  },

/* RPL_CANTBEOP */
  {"Sorry. You are not allowed to be chanop",	/* english */
    "Sorry. Het is niet toegestaan dat u kanaal operator bent",		/* dutch */
    "Désolé. Il ne vous est pas permis d'être opérateur",	/* french */
    "Lo siento. No estas autorizado para ser operador en el canal",	/* spanish */
    "Tut mir leid, aber Sie duerfen nicht Betreiber werden."	/*german */
  },

/* RPL_CANTBEOPPED */
  {"This user is not allowed to be chanop",	/* english */
    "Deze gebruiker is het niet toegestaan kanaal operator te zijn",	/* dutch */
    "Cet usager n'a pas le droit d'être opérateur",	/* french */
    "Ese usuario no está autorizado para ser operador en el canal",	/* spanish */
    "Dieser Benutzer darf nicht Betreiber werden."	/* german */
  },

/* RPL_DEOPPED1ST */
  {"You are not allowed to DEOP me!",	/* english */
    "Je mag me niet deoppen!",	/* dutch */
    "Vous ne pouvez pas me deopper!",	/* french */
    "¡No me puedes quitar el op!",	/* spanish */
    "Sie duerfen mich nicht deoppen!"	/* german */
  },

/* RPL_DEOPPED2ND */
  {"Please STOP deopping me!",	/* english */
    "STOP met me te de-oppen!",	/* dutch */
    "Veuillez arrêter de me deopper!",	/* french */
    "Por favor, ¡para de quitarme el op!",	/* spanish */
    "Bitte hoeren Sie auf, mich zu deoppen!"	/* german */
  },

/* RPL_DEOPPED3RD */
  {"I warned you!",		/* english */
    "Je was gewaarschuwd!",	/* dutch */
    "Je vous aurai prévenu!",	/* french */
    "¡Te he hecho una advertencia!",	/* spanish */
    "Sie wurden gewarnt!"	/* german */
  },

/* RPL_USERISPROTECTED */
  {"This user is protected",	/* english */
    "Deze gebruiker is beschermd",	/* dutch */
    "Cet usager est protégé",	/* french */
    "Este usuario está protegido",	/* spanish */
    "Dieser Benutzer ist geschuetzt"	/* german */
  },

/* RPL_YOUREOPPEDBY */
  {"You're opped by %s",	/* english */
    "U bent kanaal operator gemaakt door %s",	/* dutch */
    "Vous êtes oppé par %s",	/* french */
    "El %s te ha puesto de operador en el canal",	/*spanish */
    "Sie wurden von %s zum Betreiber gemacht."	/* german */
  },

/* RPL_USERNOTONCHANNEL */
  {"User %s is not on channel %s!",	/* english */
    "%s bevindt zich niet op kanaal %s!",	/* dutch */
    "%s n'est pas sur %s!",	/* french */
    "El usuario %s no está presente en el canal %s!",	/* spanish */
    "%s ist nicht auf dem Kanal %s!"	/* german */
  },

/* RPL_YOUREDEOPPEDBY */
  {"You're deopped by %s",	/* english */
    "Het kanaal operatorschap is u ontnomen door %s",	/* dutch */
    "Vous êtes déoppé par %s",	/* french */
    "El %s te quitó el status de op%s",		/* spanish */
    "%s hat ihnen den Betreiberstatus entzogen"		/* german */
  }
};
