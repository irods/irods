# Rule Engine

The Rule Engine, which keeps track of state and interprets both system-defined rules and user-defined rules, is a critical component of the iRODS system.  Rules are definitions of actions that are to be performed by the server.  These actions are defined in terms of microservices and other actions.  The iRODS built-in Rule Engine interprets the rules and calls the appropriate microservices.

## Delay execution

Rules can be run in two modes - immediate execution or delayed execution.  Most of the actions and microservices executed by the rule engine are executed immediately, however, some actions are better suited to be placed in a queue and executed later.  The actions and microservices which are to be executed in delay mode can be queued with the `delay` microservice.  Typically, delayed actions and microservices are resource-heavy, time-intensive processes, better suited to being carried out without having the user wait for their completion.  These delayed processes can also be used for cleanup and general maintenance of the iRODS system, like the cron in UNIX.

Monitoring the delayed queue is important once your workflows and maintenance scripts depends on the health of the system. The delayed queue can be managed with the following three iCommands:

1. iqdel    - remove a delayed rule (owned by you) from the queue.
2. iqmod    - modify certain values in existing delayed rules (owned by you).
3. iqstat   - show the queue status of delayed rules.

## Additional Information

More information is available in the standalone document named [Rule Language](rule_language.md)

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
