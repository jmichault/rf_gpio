#! /bin/sh
### BEGIN INIT INFO
# Provides:          rf_gpio
# Required-Start:    $network $remote_fs $syslog
# Required-Stop:     $network $remote_fs $syslog
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: RFLink sur gpio
# Description:       Ĉi tiu demono komencos la servilo «RFLink sur gpio»
### END INIT INFO

# Do NOT "set -e"

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin
DESC="RFLink on gpio server"
NAME=rf_gpio
USERNAME=pi
PIDFILE=/var/run/$NAME.pid
SCRIPTNAME=/etc/init.d/$NAME

DAEMON=/home/pi/rf_gpio/$NAME
INIFILE=/home/pi/rf_gpio/sentiloj.ini
DAEMON_ARGS="-d -h 10000 -i $INIFILE"

# Eliri se la pako ne estas instalita
[ -x "$DAEMON" ] || exit 0

# Ŝargu la agordon VERBOSE kaj aliaj rcS-variabloj
. /lib/init/vars.sh

. /lib/lsb/init-functions

pidof_rf_gpio() {
    # se efektive estas procezo rf_gpio kies pid estas en PIDFILE,
    # printi ĝin kaj redoni 0.
    if [ -e "$PIDFILE" ]; then
        if pidof rf_gpio | tr ' ' '\n' | grep -w $(cat $PIDFILE); then
            return 0
        fi
    fi
    return 1
}

#
# Funkcio kiu startas la daemon / servon
#
do_start()
{
        # Redonu
        #   0 se demono komenciĝis
        #   1 se demono jam funkciis
        #   2 se demono ne povus esti komencita
        start-stop-daemon --chuid $USERNAME --start --quiet --pidfile $PIDFILE --exec $DAEMON --test > /dev/null \
                || return 1
        start-stop-daemon --start --quiet --pidfile $PIDFILE --exec $DAEMON -- \
                $DAEMON_ARGS \
                || return 2
}

#
# Funkcio kiu haltas la daemon / servon
#
do_stop()
{
        # Redonu
        #   0 se demono estas haltita
        #   1 se demono jam estis haltita
        #   2 se demono ne povus esti haltigita
        #   alia se fiasko okazis
        start-stop-daemon --stop --quiet --retry=TERM/30/KILL/5 --pidfile $PIDFILE --name $NAME
        RETVAL="$?"
        [ "$RETVAL" = 2 ] && return 2
        # Atendu infanojn ankaŭ finiĝi, se ĉi tiu estas demono, kiu forkaŝas
        # kaj se la daemon estas nur iam ajn prilaborita de ĉi tiu initscript.
        # Se la supraj kondiĉoj ne estas kontentigitaj, aldonu alian kodon, kiu atendas
        # ke la procezo faligu ĉiujn rimedojn bezonatajn per servoj poste komencitaj.
        # Lasta rimedo estas dormi iom da tempo.
        start-stop-daemon --stop --quiet --oknodo --retry=0/30/KILL/5 --exec $DAEMON
        [ "$?" = 2 ] && return 2
        # Multaj daemonoj ne forigas siajn pidfilojn kiam ili eliras.
        rm -f $PIDFILE
        return "$RETVAL"
}

case "$1" in
  start)
        [ "$VERBOSE" != no ] && log_daemon_msg "Komencante $DESC" "$NAME"
        do_start
        case "$?" in
                0|1) [ "$VERBOSE" != no ] && log_end_msg 0 ;;
                2) [ "$VERBOSE" != no ] && log_end_msg 1 ;;
        esac
        ;;
  stop)
        [ "$VERBOSE" != no ] && log_daemon_msg "Ĉesi $DESC" "$NAME"
        do_stop
        case "$?" in
                0|1) [ "$VERBOSE" != no ] && log_end_msg 0 ;;
                2) [ "$VERBOSE" != no ] && log_end_msg 1 ;;
        esac
        ;;
  status)
        status_of_proc "$DAEMON" "$NAME" && exit 0 || exit $?
        ;;
  reload)
        log_daemon_msg "Reŝargi $DESC" "$NAME"
        PID=$(pidof_rf_gpio) || true
        if [ "${PID}" ]; then
                kill -HUP $PID
                log_end_msg 0
        else
                log_end_msg 1
        fi
        ;;
  restart)
        log_daemon_msg "Relanĉante $DESC" "$NAME"
        do_stop
        case "$?" in
          0|1)
                do_start
                case "$?" in
                        0) log_end_msg 0 ;;
                        1) log_end_msg 1 ;; # Malnova procezo ankoraŭ funkcias
                        *) log_end_msg 1 ;; # Malsukcesis komenci
                esac
                ;;
          *)
                # Malsukcesis ĉesi
                log_end_msg 1
                ;;
        esac
        ;;
  *)
        echo "Usage: $SCRIPTNAME {start|stop|status|restart|reload}" >&2
        exit 3
        ;;
esac

:
