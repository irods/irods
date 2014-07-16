#!/bin/bash
supervisorctl start sshd
supervisorctl start postgresql
supervisorctl start irodsServer
sudo su -c "iadmin modresc demoResc host $HOSTNAME" irods


