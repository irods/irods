#version 0.2

FROM ubuntu:14.04
MAINTAINER danb@renci.org
RUN apt-get update
RUN apt-get upgrade -y
RUN locale-gen en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US.UTF-8
ENV LC_ALL en_US.UTF-8

RUN DEBIAN_FRONTEND=noninteractive apt-get install -y openssh-server supervisor postgresql-9.3 wget dpkg sudo libcurl4-gnutls-dev

RUN mkdir -p /var/run/sshd
RUN mkdir -p /var/log/supervisor
ADD supervisord.conf /etc/supervisor/conf.d/supervisord.conf
ADD supervisord.conf.etc /etc/supervisor/supervisord.conf

RUN service postgresql start && \
  sudo -u postgres createdb -O postgres 'ICAT' -E UTF8 -l en_US.UTF-8 -T template0 && \
  sudo -u postgres psql -U postgres -d postgres -c "CREATE USER irods WITH PASSWORD 'testpassword'" && \
  sudo -u postgres psql -U postgres -d postgres -c 'GRANT ALL PRIVILEGES ON DATABASE "ICAT" TO irods'

RUN useradd admin
RUN echo 'admin:admin' | chpasswd
RUN mkdir /home/admin
RUN chown admin:admin /home/admin
RUN chsh -s /bin/bash admin

RUN wget -P /home/admin ftp://ftp.renci.org/pub/irods/releases/4.0.2/irods-database-plugin-postgres-1.2.deb
RUN wget -P /home/admin ftp://ftp.renci.org/pub/irods/releases/4.0.2/irods-icat-4.0.2-64bit.deb

# install package dependencies to prevent Docker build from erring out
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y `dpkg -I /home/admin/irods-icat-4.0.2-64bit.deb | sed -n 's/^ Depends: //p' | sed 's/,//g'`
RUN dpkg -i /home/admin/irods-icat-4.0.2-64bit.deb

RUN DEBIAN_FRONTEND=noninteractive apt-get install -y `dpkg -I /home/admin/irods-database-plugin-postgres-1.2.deb | sed -n 's/^ Depends: //p' | sed 's/,//g'`
RUN dpkg -i /home/admin/irods-database-plugin-postgres-1.2.deb

ADD db.cfg /home/admin/db.cfg
RUN mv /var/lib/irods/iRODS/server/bin/irodsServer /var/lib/irods/iRODS/server/bin/irodsServer.actual
ADD irodsServer.sham /var/lib/irods/iRODS/server/bin/irodsServer
RUN chmod a+x /var/lib/irods/iRODS/server/bin/irodsServer
RUN chown irods:irods /var/lib/irods/iRODS/server/bin/irodsServer

# irods needs to be part of admin to execute supervisorctl
RUN usermod -G admin -a irods

ADD server.sh /home/admin/server.sh
ADD runAll.sh /home/admin/runAll.sh
RUN chmod a+x /home/admin/server.sh
RUN chmod a+x /home/admin/runAll.sh

RUN /usr/bin/supervisord && \
  supervisorctl start postgresql && \
  sudo su -c "cat /home/admin/db.cfg | /var/lib/irods/packaging/setup_database.sh" irods

RUN sed -i 's/^irodsHost.*/irodsHost localhost/' /var/lib/irods/.irods/.irodsEnv

EXPOSE 22 1247
ENTRYPOINT /usr/bin/supervisord "-n"

