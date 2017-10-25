#! /bin/sh
### BEGIN INIT INFO
# Provides:          mysql-honeypot
# Required-Start:    $remote_fs $syslog
# Required-Stop:     $remote_fs $syslog
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: MySQL Honeypot
### END INIT INFO

# Author: Volodymyr Kolesnykov <volodymyr@wildwolf.name>

PATH=/sbin:/usr/sbin:/bin:/usr/bin
DESC="MySQL Honeypot"
NAME=mysql-honeypotd
DAEMON=/usr/sbin/$NAME
PIDFILE=/run/mysql-honeypotd/$NAME.pid
DAEMON_ARGS=
SCRIPTNAME=/etc/init.d/$NAME

# Defaults, do no touch
ENABLED=0

[ -x "$DAEMON" ] || exit 0
[ -r /etc/default/$NAME ] && . /etc/default/$NAME

DAEMON_ARGS="$DAEMON_ARGS -P $PIDFILE"

if [ -z "$DAEMON_USER" ]; then
	DAEMON_USER=daemon
fi

if [ -z "$DAEMON_GROUP" ]; then
	DAEMON_GROUP=daemon
fi

DEAMON_ARGS="$DAEMON_ARGS -u $DAEMON_USER -g $DAEMON_GROUP"

mkdir -p $(dirname "$PIDFILE")
chmod 0770 $(dirname "$PIDFILE")
chown "root:$DAEMON_GROUP" $(dirname "$PIDFILE")

. /lib/init/vars.sh
. /lib/lsb/init-functions

check_enabled()
{
	if [ "$ENABLED" = "0" ]; then
		echo "$DESC: disabled, see /etc/default/$NAME"
		exit 0
	fi
}

do_start()
{
	# Return
	#   0 if daemon has been started
	#   1 if daemon was already running
	#   2 if daemon could not be started
	start-stop-daemon --start --quiet --pidfile $PIDFILE --exec $DAEMON --test > /dev/null \
		|| return 1
	start-stop-daemon --start --quiet --pidfile $PIDFILE --exec $DAEMON -- $DAEMON_ARGS \
		|| return 2
}

do_stop()
{
	# Return
	#   0 if daemon has been stopped
	#   1 if daemon was already stopped
	#   2 if daemon could not be stopped
	#   other if a failure occurred
	start-stop-daemon --stop --quiet --retry=TERM/30/KILL/5 --pidfile $PIDFILE --name $NAME
	RETVAL="$?"
	[ "$RETVAL" = 2 ] && return 2
	rm -f $PIDFILE
	return "$RETVAL"
}

case "$1" in
	start)
		check_enabled
		[ "$VERBOSE" != no ] && log_daemon_msg "Starting $DESC" "$NAME"
		do_start
		case "$?" in
			0|1) [ "$VERBOSE" != no ] && log_end_msg 0 || exit 0 ;;
			2) [ "$VERBOSE" != no ] && log_end_msg 1 || exit 1 ;;
		esac
		;;
	stop)
		[ "$VERBOSE" != no ] && log_daemon_msg "Stopping $DESC" "$NAME"
		do_stop
		case "$?" in
			0|1) [ "$VERBOSE" != no ] && log_end_msg 0 || exit 0 ;;
			2) [ "$VERBOSE" != no ] && log_end_msg 1 || exit 1 ;;
		esac
		;;
	status)
		status_of_proc "$DAEMON" "$NAME" && exit 0 || exit $?
		;;
	restart|force-reload)
		check_enabled
		log_daemon_msg "Restarting $DESC" "$NAME"
		do_stop
		case "$?" in
			0|1)
				do_start
				case "$?" in
					0) log_end_msg 0 ;;
					1) log_end_msg 1 ;; # Old process is still running
					*) log_end_msg 1 ;; # Failed to start
				esac
				;;
			*)
				# Failed to stop
				log_end_msg 1
				;;
		esac
		;;
	*)
		echo "Usage: $SCRIPTNAME {start|stop|status|restart|force-reload}" >&2
		exit 3
		;;
esac
