FROM debian:bullseye

ARG USERNAME
ARG ARCH
ARG CROSSBUILD

RUN useradd -m $USERNAME
RUN usermod -aG sudo $USERNAME
RUN usermod --shell /bin/bash $USERNAME

RUN apt-get update -q
RUN apt-get upgrade -q -y
RUN apt-get install -y\
    sudo\
    bash-completion\
    build-essential\
    gnupg2\
    gdb\
    gdb-multiarch\
    rsync\
    make\
    cmake\
    git\
    nano\
    avahi-utils\
    iputils-ping\
    cmake\
    x11-apps\
    libgl1-mesa-dev\
    xorg-dev

RUN dpkg --add-architecture $ARCH
RUN apt-get update -q
RUN apt-get install -y \
    crossbuild-essential-$ARCH

RUN mkdir -p /etc/sudoers.d/
RUN echo "$USERNAME ALL=(ALL:ALL) NOPASSWD: ALL" >> /etc/sudoers.d/$USERNAME
RUN chmod 440 /etc/sudoers.d/$USERNAME
RUN touch /var/lib/sudo/lectured/$USERNAME

ENV PKG_CONFIG_PATH=/usr/lib/${CROSSBUILD}/pkgconfig:/usr/share/pkgconfig
ENV PKG_CONFIG_LIBDIR=/usr/lib/${CROSSBUILD}/pkgconfig:/usr/share/pkgconfig
ENV DISPLAY=:0
ENV ARCH=$ARCH
ENV CC=${CROSSBUILD}-gcc
ENV CXX=${CROSSBUILD}-g++

USER $USERNAME