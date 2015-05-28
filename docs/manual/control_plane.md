# Control Plane

A fundamental concept within iRODS is virtualization, the separation of the logical from the physical.  This separation is provided by iRODS along several different axes: storage, networking, authentication, and other plugins as well as the iRODS logical namespace itself. To further refine terminology around this concept, we have separated the use of the terms Zone and Grid into two distinct entities. We now use the term Zone to refer to the purely logical portion of an iRODS deployment and the term Grid to describe the purely physical. An iRODS Zone provides services to clients via the iRODS API and the wire-line protocol.

![Control Plane Diagram](../ControlPlaneDiagram.jpg)

Previously, an iRODS Grid had no means of communicating with the outside world.  There was no mechanism to gracefully shut down a grid, preventing new connections while allowing existing connections to complete their transactions. There was no means to pause the grid to install new plugins, and no way to resume afterward.  And there was no way to query the health of the grid itself.

To fill in these gaps, we have created a new communication channel to allow data grid administrators to control an iRODS Grid from the command line. We have created a new command line tool, `irods-grid`, which will communicate directly with a new thread within the irodsServer, which will service these new requirements.

## The `irods-grid` Command

The control plane is the first addition to iRODS which leverages [ZeroMQ](http://zeromq.org/) and [Avro](https://avro.apache.org/), new technologies that will continue to be integrated as standard interfaces within iRODS going forward.  The `irods-grid` command is a standalone client to the control plane, but much like the iRODS API itself, the control plane may be reached by any other system which interfaces with ZeroMQ at the network layer and uses Avro for serialization.

There are currently four different actions, or subcommands, that the control plane supports, which may target either one or more hosts or the entire grid. These actions will timeout after a given number of seconds, or block forever.

The `irods-grid` command takes the following parameters:


```
irods@hostname:~/ $ irods-grid --help
usage: 'irods-grid action [ option ] target'
action: ( required ) status, pause, resume, shutdown
option: --force-after=seconds or --wait-forever
target: ( required ) --all, --hosts=", , ..."
```

For example, if an administrator wanted to query the status of a single server, the command would look like:


```
irods@hostname:~/ $ irods-grid status --host=host.example.org
```

and the output would look like:


```
{
  "hosts": [
    {
      "hostname": "host.example.org",
      "irods_server_pid": 15399,
      "agents": [
         {
           "agent_pid": 22192,
           "age": 0
         }
      ],
      "xmsg_server_pid": 0,
      "status": "server_state_running",
      "re_server_pid": 15401
    }
  ]
}
```

If an administrator wanted to pause the entire grid, do some updates, and then resume it later the commands would be:


```
irods@hostname:~/ $ irods-grid pause --all
```

and then:

```
irods@hostname:~/ $ irods-grid resume --all
```

The control plane will then reach out to the iCAT server and request all the unique host names for every resource server in order to create the list of hosts to reach for forwarding the command.  Should a smaller list of hosts be required, the fully qualified domain names can be listed as follows:


```
irods@hostname:~/ $ irods-grid pause --hosts="host1.example.org, host2.example.org"
```

If a server is particularly stuck, the administrator can use the --force-after switch to force the operation after a given grace period.  Or if a particularly important job is in flight, the command can block forever using the --wait-forever switch.

## Configuration and Security

The `irods-grid` command is purely within the control of a data-grid administrator. For this reason we decided to secure this side-channel communication with symmetric grid-wide keys. This way the only way a grid may be paused, shutdown or queried is by an administrator with the proper credentials. For the client side, the `irods_environment.json` holds the port, key, and information about the encryption methodology:


```
"irods_server_control_plane_port": 1248,
"irods_server_control_plane_key": "TEMPORARY__32byte_ctrl_plane_key",
"irods_server_control_plane_encryption_num_hash_rounds": 16,
"irods_server_control_plane_encryption_algorithm": "AES-256-CBC"
```

And for the server side of the configuration (`server_config.json`), which needs to match the clients as well as all of the servers, we have a very similar configuration:


```
"server_control_plane_port": 1248,
"server_control_plane_key": "TEMPORARY__32byte_ctrl_plane_key",
"server_control_plane_encryption_num_hash_rounds": 16,
"server_control_plane_encryption_algorithm": "AES-256-CBC",
"server_control_plane_timeout_milliseconds": 10000
```

As the control plane works grid-wide, the server handling the request from `irods-grid` will forward a command when requested. This is the motivation for the addition of the timeout; If a server doesn't respond within an appropriate time frame, it is assumed to be down. This is an important point of tuning if a grid has very remote servers.

