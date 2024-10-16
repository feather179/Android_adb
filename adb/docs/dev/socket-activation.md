```
adb can be configured to work with systemd-style socket activation,
allowing the daemon to start automatically when the adb control port
is forwarded across a network. You need two files, placed in the usual
systemd service directories (e.g., ~/.config/systemd/user for a user
service).

adb.service:

--- START adb.service CUT HERE ---
[Unit]
Description=adb
After=adb.socket
Requires=adb.socket
[Service]
Type=simple
# FD 3 is part of the systemd interface
ExecStart=/path/to/adb server nodaemon -L acceptfd:3
--- END adb.service CUT HERE ---

--- START adb.socket CUT HERE ---
[Unit]
Description=adb
PartOf=adb.service
[Socket]
ListenStream=127.0.0.1:5037
Accept=no
[Install]
WantedBy=sockets.target
--- END adb.socket CUT HERE ---

After installing the adb service, the adb server will be started
automatically on any connection to 127.0.0.1:5037 (the default adb
control port), even after adb kill-server kills the server.

Other "superserver" launcher systems (like macOS launchd) can be
configured analogously. The important part is that adb be started with
"server" and "nodaemon" command line arguments and that the listen
address (passed to -L) name a file descriptor that's ready to
accept(2) connections and that's already bound to the desired address
and listening. inetd-style pre-accepted sockets do _not_ work in this
configuration: the file descriptor passed to acceptfd must be the
serve socket, not the accepted connection socket.
```