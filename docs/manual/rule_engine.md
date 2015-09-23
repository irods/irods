# Rule Engine

The Rule Engine, which keeps track of state and interprets both system-defined rules and user-defined rules, is a critical component of the iRODS system.  Rules are definitions of actions that are to be performed by the server.  These actions are defined in terms of microservices and other actions.  The iRODS built-in Rule Engine interprets the rules and calls the appropriate microservices.

## Delay execution

Rules can be run in two modes - immediate execution or delayed execution.  Most of the actions and microservices executed by the rule engine are executed immediately, however, some actions are better suited to be placed in a queue and executed later.  The actions and microservices which are to be executed in delay mode can be queued with the `delay` microservice.  Typically, delayed actions and microservices are resource-heavy, time-intensive processes, better suited to being carried out without having the user wait for their completion.  These delayed processes can also be used for cleanup and general maintenance of the iRODS system, like the cron in UNIX.

Monitoring the delayed queue is important once your workflows and maintenance scripts depends on the health of the system. The delayed queue can be managed with the following three iCommands:

1. iqdel    - remove a delayed rule (owned by you) from the queue.
2. iqmod    - modify certain values in existing delayed rules (owned by you).
3. iqstat   - show the queue status of delayed rules.

### Syntax

The `delay` microservice is invoked with the following syntax:

~~~c
delay("hints") {
        microservice-chain_part1 ::: recovery-microservice-chain_part1;
        microservice-chain_part2 ::: recovery-microservice-chain_part2;
        microservice-chain_part3 ::: recovery-microservice-chain_part3;
        .
        .
        .
        microservice-chain_partN ::: recovery-microservice-chain_partN;
   }
~~~

"hints" (required) are of the form:

  - `ET` - Execution Time - Absolute time (without time zones) when the delayed execution should be performed. The input can be incremental time given in:
    - `nnnn` - an integer - assumed to be in seconds
    - `nnnnU` - `nnnn` is an integer, `U` is the unit (s-seconds, m-minutes, h-hours, d-days, y-years)
    - `dd.hh:mm:ss` - 2-digit integers representing days, hours, minutes, and seconds, respectively. Most significant values can be omitted (e.g. 20:40 means mm:ss)
    - `YYYY-MM-DD.hh:mm:ss` - Least significant values can be omitted (e.g. 2015-07-29.12 means noon of July 29, 2015)
  - `PLUSET` - Relative Execution Time - Relative to current time when the delayed execution should be performed.
  - `EF` - Execution Frequency - How often the delayed execution should be performed. The `EF` value is of the form:
    - `nnnnU <directive>` where
        - `nnnn` is an integer, `U` is the unit (s-seconds, m-minutes, h-hours, d-days, y-years)
        - `<directive>` can be of the form:
            - `<empty-directive>` - equivalent to `REPEAT FOR EVER`
            - `REPEAT FOR EVER`
            - `REPEAT UNTIL SUCCESS`
            - `REPEAT nnnn TIMES` - `nnnn` is an integer
            - `REPEAT UNTIL <time>` - `<time>` is of the form `YYYY-MM-DD.hh:mm:ss`
            - `REPEAT UNTIL SUCCESS OR UNTIL <time>`
            - `REPEAT UNTIL SUCCESS OR nnnn TIMES`
            - `DOUBLE FOR EVER`
            - `DOUBLE UNTIL SUCCESS`
            - `DOUBLE nnnn TIMES`
            - `DOUBLE UNTIL <time>`
            - `DOUBLE UNTIL SUCCESS OR UNTIL <time>`
            - `DOUBLE UNTIL SUCCESS OR nnnn TIMES`

### Examples

This example will queue the chain of microservices to begin in 1 minute and repeat every 20 minutes forever:

~~~c
delay("<PLUSET>1m</PLUSET><EF>20m</EF>") {
    writeLine("serverLog", " -- Delayed Execution");
}
~~~

This example will queue the chain of microservices to begin in 1 minute and repeat every 20 minutes forever:

~~~c
delay("<PLUSET>1m</PLUSET><EF>20m</EF>") {
    writeLine("serverLog", " -- Delayed Execution");
}
~~~


## Remote execution

A microservice chain can be executed on a remote iRODS server. This gives the flexibility to 'park' microservices where it is most optimal. For example, if there is a microservice which needs considerable computational power, then performing it at a compute-intensive site would be appropriate. Similarly, if one is computing checksums, performing it at the server where the data is located would be more appropriate.

### Syntax

The `remote` microservice is invoked with the following syntax:

~~~c
remote("host","hints") {
        microservice-chain_part1 ::: recovery-microservice-chain_part1;
        microservice-chain_part2 ::: recovery-microservice-chain_part2;
        microservice-chain_part3 ::: recovery-microservice-chain_part3;
        .
        .
        .
        microservice-chain_partN ::: recovery-microservice-chain_partN;
   }
~~~

"host" (required) is the hostname on which the remote execution should be performed.

"hints" (required) are of the form:

  - `ZONE` - Remote Zone - The name of the Zone in which the "host" is located.

### Examples

This example will execute the chain of microservices on the host "resource.example.org" in the local Zone:

~~~c
remote("resource.example.org","") {
    writeLine("serverLog", " -- Remote Execution in Local Zone");
}
~~~

This example will execute the chain of microservices on the host "farawayicat.example.org" in the remote Zone named "DifferentZone":

~~~c
remote("farawayicat.example.org","<ZONE>DifferentZone</ZONE>") {
    writeLine("serverLog", " -- Remote Zone Execution");
}
~~~

The best practice for using both `delay()` and `remote()` [depends on the use case](best_practices.md#using-both-delay-and-remote-execution).


<!--
..
.. ---------------
.. Delay Execution
.. ---------------
.. - how
.. - what
.. - when
.. - where
.. - why
.. - errors
.. - queue management
.. - file locking
..
.. ----------
.. Monitoring
.. ----------
.. - nagios plugins (Jean-Yves)
.. - other
.. - Failover checking
..
-->
