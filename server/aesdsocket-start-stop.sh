#! /bin/sh 
case "$1" in 
 start) 
 echo "Starting simpelserver" 
 start-stop-daemon --oknodo -S -n aesdsocket -a /usr/bin/aesdsocket -- -d
 #start-stop-daemon --oknodo -S -n aesdsocket -a /usr/bin/aesdsocket --
 #start-stop-daemon --oknodo -S -n aesdsocket -a $(pwd)/aesdsocket -- -d
 status=$?
 if [ "$status" -eq 0 ]; then
     echo "OK"
 else
     echo "FAIL"
 fi
 return "$status"
 ;; 
 stop) 
 echo "Stopping simpleserver" 
 start-stop-daemon -K -n aesdsocket 
 ;; 
 *) 
 echo "Usage: $0 {start|stop}" 
 exit 1 
esac 
exit 0
