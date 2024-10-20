```
Implementation notes regarding ADB.

I. General Overview:

The Android Debug Bridge (ADB) is used to:

- keep track of all Android devices and emulators instances
  connected to or running on a given host developer machine

- implement various control commands (e.g. "adb shell", "adb pull", etc.)
  for the benefit of clients (command-line users, or helper programs like
  DDMS). These commands are called 'services' in ADB.

As a whole, everything works through the following components:

  1. The ADB server

    This is a background process that runs on the host machine. Its purpose
    is to sense the USB ports to know when devices are attached/removed,
    as well as when emulator instances start/stop.

    It thus maintains a list of "connected devices" and assigns a 'state'
    to each one of them: OFFLINE, BOOTLOADER, RECOVERY or ONLINE (more on
    this below).

    The ADB server is really one giant multiplexing loop whose purpose is
    to orchestrate the exchange of data (packets, really) between clients,
    services and devices.


  2. The ADB daemon (adbd)

    The 'adbd' program runs as a background process within an Android device
    or emulated system. Its purpose is to connect to the ADB server
    (through USB for devices, through TCP for emulators) and provide a
    few services for clients that run on the host.

    The ADB server considers that a device is ONLINE when it has successfully
    connected to the adbd program within it. Otherwise, the device is OFFLINE,
    meaning that the ADB server detected a new device/emulator, but could not
    connect to the adbd daemon.

    The BOOTLOADER and RECOVERY states correspond to alternate states of
    devices when they are in the bootloader or recovery mode.

  3. The ADB command-line client

    The 'adb' command-line program is used to run adb commands from a shell
    or a script. It first tries to locate the ADB server on the host machine,
    and will start one automatically if none is found.

    Then, the client sends its service requests to the ADB server.

    Currently, a single 'adb' binary is used for both the server and client.
    this makes distribution and starting the server easier.


  4. Services

    There are essentially two kinds of services that a client can talk to.

    Host Services:
      These services run within the ADB Server and thus do not need to
      communicate with a device at all. A typical example is "adb devices"
      that is used to return the list of currently known devices and their
      states. There are a few other services, though.

    Local Services:
      These services either run within the adbd daemon, or are started by
      it on the device. The ADB server is used to multiplex streams
      between the client and the service running in adbd. In this case
      its role is to initiate the connection, then of being a pass-through
      for the data.


II. Protocol details:

  1. Client <-> Server protocol:

    This section details the protocol used between ADB clients and the ADB
    server itself. The ADB server listens on TCP:localhost:5037.

    A client sends a request using the following format:

        1. A 4-byte hexadecimal string giving the length of the payload
        2. Followed by the payload itself.

    For example, to query the ADB server for its internal version number,
    the client will do the following:

        1. Connect to tcp:localhost:5037
        2. Send the string "000Chost:version" to the corresponding socket

    The 'host:' prefix is used to indicate that the request is addressed
    to the server itself (we will talk about other kinds of requests later).
    The content length is encoded in ASCII for easier debugging.

    The server should answer a request with one of the following:

        1. For success, the 4-byte "OKAY" string

        2. For failure, the 4-byte "FAIL" string, followed by a
           4-byte hex length, followed by a string giving the reason
           for failure.

    Note that the connection is still alive after an OKAY, which allows the
    client to make other requests. But in certain cases, an OKAY will even
    change the state of the connection.

    For example, the case of the 'host:transport:<serialnumber>' request,
    where '<serialnumber>' is used to identify a given device/emulator; after
    the "OKAY" answer, all further requests made by the client will go
    directly to the corresponding adbd daemon.

    The file SERVICES.TXT lists all services currently implemented by ADB.


  2. Transports:

    An ADB transport models a connection between the ADB server and one device
    or emulator. There are currently two kinds of transports:

       - USB transports, for physical devices through USB

       - Local transports, for emulators running on the host, connected to
         the server through TCP

    In theory, it should be possible to write a local transport that proxies
    a connection between an ADB server and a device/emulator connected to/
    running on another machine. This hasn't been done yet though.

    Each transport can carry one or more multiplexed streams between clients
    and the device/emulator they point to. The ADB server must handle
    unexpected transport disconnections (e.g. when a device is physically
    unplugged) properly.
```