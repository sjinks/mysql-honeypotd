#ifndef DFA_H
#define DFA_H

struct connection_t;

int handle_new_connection(struct connection_t* conn, int mask);
int handle_write(struct connection_t* conn, int mask, int next);
int handle_auth(struct connection_t* conn, int mask);


#endif /* DFA_H */
