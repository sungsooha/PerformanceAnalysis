FROM ubuntu:latest


# install ADIOS
RUN \
  mkdir -p /Chimbuko

WORKDIR /Chimbuko

RUN \ 
  apt-get update && \
  apt-get install -y build-essential wget openmpi-bin libopenmpi-dev g++ gfortran python3 python3-pip

RUN \
  wget https://users.nccs.gov/~pnorbert/adios-1.13.1.tar.gz && \
  tar xvfz adios-1.13.1.tar.gz && \
  cd adios-1.13.1 && \
  mkdir -p build  && \
  cd build/ && \
  ../configure -prefix=/opt/adios1/1.13.1/gnu/openmpi CFLAGS="-fPIC" && \
  make && \
  make install 


# install python
ENV PATH=${PATH}:/opt/adios1/1.13.1/gnu/openmpi/bin

RUN \
  pip3 install --upgrade pip && \
  echo "pip3 install configparser " | bash && \
  echo "pip3 install numpy " | bash &&\
  echo "pip3 install scipy sklearn adios adios_mpi" | bash

RUN \
  apt-get install -y vim python3-tk && \
  echo "pip3 install matplotlib " | bash
  

# Copy current directory contents into the container
ADD \
  . /Chimbuko
