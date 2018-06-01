FROM ubuntu:16.04
RUN apt-get update -y && apt-get install apt-utils -y && apt-get install locales -y

# Set the locale
RUN locale-gen en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8

# Set frontend
ENV DEBIAN_FRONTEND noninteractive

# Install dependencies
RUN apt-get update -y && apt-get dist-upgrade -y
RUN apt-get install -y \
  git \
  build-essential \
  cmake \ 
  libboost-all-dev \
  jam \
  graphviz \
  gnuplot \
  doxygen-gui \
  ant \
  swig \
  libantlr3c-dev \
  gdal-bin \
  libhdf5-dev \
  xdot \
  hdf5-tools \
  openjdk-8-jdk\
  nano \
  python-pip \
  python-tk

# Set Java variables
RUN export JAVA_HOME=/usr/java/java-8-openjdk-amd64 && export PATH=${PATH}:${JAVA_HOME}/bin

# Install python dependencies
RUN pip install --upgrade pip
RUN python -m pip install geographiclib matplotlib

# Set entrypoint
RUN mkdir -p /usr/local/share/trex
RUN touch /usr/local/share/trex/trex_init.bash
RUN echo "source /usr/local/share/trex/trex_init.bash" >> /etc/bash.bashrc