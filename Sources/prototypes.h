/* @(#)$Id: prototypes.h,v 1.20 2000/10/24 15:15:53 seks Exp $ */

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

void log(char *);
void SpecLog(char *);
void LogChan(void);
int connection(char *);
void try_later(char *);
void sendtoserv(char *);
int wait_msg(void);
void dumpbuff(void);
void rec_sigsegv(int);
void rec_sigbus(int);
void rec_sigterm(int);
void rec_sigint(int);
void dumpcore(char *);
void show_rusage(char *);
void regist(void);
void signon(void);
void onserver(char *,char *,char *);
void onsettime(char *,char *);
void onsquit(char *,char *,char *);
aserver *ToServer(char *);
aserver **FindServer(aserver **, char *);
char *ToWord(int,char *);
void GetWord(int,char *,char *);
char *time_remaining(time_t);
void proc(char *,char *, char *, char *);
void pong(char *);
void showversion(char *);
int reconnect(char *server);
int xmatch(char *, char *);
int compare(char *, char *);
int match(char *, char *);
int regex_cmp(char *patern, char *string);
int mycasecmp(char *,char *);
int key_match(char *,char *[]);
void string_swap(char *, size_t, char *, char *);
int quit(char *, int);
int restart(char *); /* added for restart function -Kev */
void showcommands(char *,char *,char *);
void showhelp(char *,char *,char *);
void showmotd(char *);
void ShowChanInfo(char *,char *,char *);
void isreg(char *,char *,char *);
void LoadUserList(char *);
void SaveUserList(char *,char *);
void DelChannel(char *);
void FreeUser(auser *);
void NewChannel(char *,time_t,int);
void QuitAll(void);
void privmsg(char *,char *,char *);
void parse_command(char *,char *,char *,char *);
int GuessChannel(char *, char *);
void try_find(char *,aluser *);
int LAccess(char *,aluser *);
int Access(char *,char *);
void verify(char *, char *);
RegUser *IsValid(aluser *,char *);
int IsShit(char *,char *,char *, char *);
void AddUser(char *,char *,char *);
void showaccess(char *,char *,char *);
void RemoveUser(char *,char *,char *);
void ModUserInfo(char *,char *,char *,char *);
void purge(char *,char *,char *);
void ChPass(char *,char *,char *);
void SetChanFlag(char *,char *,char *);
void free_user(RegUser **);
void validate(char *,char *,char *);
void DeAuth(char *, char *, char *);
void join(char *,char *,char *);
void joindefault(void);
void SendBurst(void);
void invite(char *,char *,char *);
void part(char *,char *,char *);
void oninvite(char *,char *);
void onjoin(char *,char *);
void onpart(char *,char *);
void onkick(char *,char *,char *);
void onop(char *,char *,char *);
void ondeop(char *,char *,char *,int *);
void GetOps(char *);
int IsOpless(char *);
void onopless(char *);
void onnick(char *,char *,char *);
void onban(char *,char *,char *);
void onunban(char *,char *,char *);
void showbanlist(char *,char *,char *);
void AddBan(char *,char *);
void RemBan(char *,char *);
void onwhois(char *,char *);
void UserQuit(char *);
achannel *ToChannel(char *);
auser *ToUser(char *,char *);
aluser *ToLuser(char *);
void onquit(char *);
void onkill(char *,char *,char *);
void onwho(char *);
void showusers(char *);	
void showchannels(void);
void setchanmode(char *);
void ModeChange(char *,char *,char *);
void changemode(char *,char *,char *,int);
void flushmode(char *);
void bounce(char *,char *,time_t);
int IsSet(char *,char,char *);
void op(char *,char *,char *);
void deop(char *,char *,char *);
void massdeop(char *);
void MakeBanMask(aluser *, char *);
void ban(char *,char *,char *);
void mban(char *,char *,char *);
void unban(char *,char *,char *);
void kick(char *,char *,char *);
void topic(char *,char *,char *);
void notice(char *,char *);
void servnotice(char *,char *);
void broadcast(char *,int);
void NickInUse(void);
void ChNick(char *);
void AddToShitList(char *, char *, char *,int);
void RemShitList(char *,char *,char *,int);
void CleanShitList(char *,char *);
void suspend(char *, char *, char *);
void unsuspend(char *, char *, char *);
void ShowShitList(char *,char *,char *);
void LoadDefs(char *);
void SearchChan(char *,char *,char *);
void AddChan(char *,char *,char *);
void SaveDefs(char *);
void RemChan(char *, char *, char *);
int CheckFlood(char *,char *,int);
int CheckAdduserFlood(char *, char *);
void ontopic(char *,char *,char *);
void onnotice(char *,char *,char *);
void LoadShitList(char *);
void SaveShitList(char *,char *);
void parse_ctcp(char *,char *,char *);
void showstatus(char *,char *,char *);
void showmap(char *);
void showserv(char *,aserver *, int *);
void InitEvent(void);
void AddEvent(int, time_t, char *);
void CheckEvent(void);
void CleanIgnores(void);
void AddIgnore(char *,char *,int);
int CheckPrivateFlood(char *,int,char *);
int CheckFloodFlood(char *,int);
int IsIgnored(char *);
void ShowIgnoreList(char *,char *,char *);
void AdminRemoveIgnore(char *, char *, char *);
void CalmDown(char *,char *,char *);
void OperJoin(char *,char *,char *);
void OperPart(char *,char *,char *);
void ClearMode(char *, char *, char *);
void ReplyNotAccess(char *,char *);
void CheckIdleChannels(void);
void RandomChannel(char *);
void Say(char *,char *);
void RobinSay(char *,char *);
void ServNotice(char *,char *);
int IsReg(char *);
#ifdef FAKE_UWORLD
void IntroduceUworld(void);
void KillUworld(char *);
void Uworld_switch(char *,char *,char *);
void parse_uworld_command(char *,char *);
void Uworld_opcom(char *,char *);
void ClearChan(char *, char *);
void Uworld_reop(char *, char *);
void OperSuspend(char *, char *);
int IsOperSuspended(char *);
#endif
#ifdef NICKSERV
void IntroduceNickserv(void);
#endif
#ifdef DOHTTP
void open_http(void);
void remove_httpsock(http_socket *old);
void http_accept(int sock);
void parse_http(http_socket *hsock, char *buf);
void readfrom_file(http_file_pipe *fpipe);
void destroy_file_pipe(http_file_pipe *old);
void read_http_conf(char *);
long sendto_http(http_socket *sck, char *format, ...);
int readfrom_http(http_socket *);
int flush_http_buffer(http_socket *);
#endif
void upgrade(char *,char *);
void open_patch_socket(char *);
int readfrom_misc(misc_socket *);
int flush_misc_buffer(misc_socket *);
long sendto_misc(misc_socket *, char *, ...);
void send_misc_handshake(misc_socket *);

struct buffer_block *get_buffer_block(void);
void return_buffer_block(struct buffer_block *);
int copy_from_buffer(struct buffer_block **, char *, char, int);
int look_in_buffer(struct buffer_block **, char *, char, int);
long copy_to_buffer(struct buffer_block **, char *,int);
int zap_buffer(struct buffer_block **);
int find_char_in_buffer(struct buffer_block **, char,int);
int skip_char_in_buffer(struct buffer_block **, int);
char *make_dbfname(char *);
int db_fetch(char *,unsigned int,char *,char *,int,void *,void *,DBCALLBACK(x));
void read_db(dbquery *);
void end_db_read(dbquery *);
void db_sync(char *);
void db_sync_ready(dbsync *);
void end_db_sync(dbsync *);
void db_test(char *, char *,char *);
void gather_sync_channels(void);
void sync_next_channel(void);
void do_cold_sync(void);
void do_cold_sync_slice(void);
#ifdef HISTORY
void History(char *);
#endif


int ul_hash(char *);
int sl_hash(char *);
int lu_hash(char *);
int cl_hash(char *);
int su_hash(char *);


#ifdef NICKSERV
void IntroduceNickserv(void);
void KillNickserv(char *msg);
void nserv_nickserv(char *source, char *args);
void nserv_addnick(char *source, char *args);
void nserv_addmask(char *source, char *args);
void nserv_remnick(char *source, char *args);
void nserv_remmask(char *source, char *args);
void nserv_nickinfo(char *source, char *args);
void nserv_identify(char *source, char *args);
void nserv_ghost(char *source, char *args);
void nserv_nicknewpass(char *source, char *args);
void nserv_nicknewemail(char *source, char *args);
void nserv_save(void);
void nserv_load(void);
void nserv_checkregnick(char *nick);
void nserv_quit(aluser *user);
void nserv_nick(char *newnick, aluser *user);
void nserv_onop(char *channel, auser *user);
#endif
void DccMe(char *, char *);
