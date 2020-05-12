#ifndef DFA_H
#define DFA_H

struct connection_t;
struct ev_loop;
struct ev_timer;

int handle_new_connection(struct connection_t* conn, int mask);
int handle_write(struct connection_t* conn, int mask, int next);
void do_auth_failed(struct ev_loop* loop, struct ev_timer* timer, int revents);
int handle_auth(struct connection_t* conn, int mask);

#endif /* DFA_H */
