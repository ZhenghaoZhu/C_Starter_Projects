/*  SECTION  When disconnection of the client is noticed by the client service thread, the corresponding player is logged
out of the server and the client service thread terminates. Any outstanding invitations to games held by the now-logged-out player
are revoked or declined, and games in progress involving that player are resigned.  Information about the player remains in the system;
in the present implementation this consists of the player's name and rating. */

void* mainListeningThread(void *vargp);
void handle_SIGHUP(int signum);