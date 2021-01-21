FROM ubuntu:20.10

MAINTAINER Taylor R. Brown <tbrown122387@gmail.com>

# Set Env vars
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=America/New_York

# copy all this cpp code into container
WORKDIR /usr/src/app
COPY . .

# install cpp dependencies
RUN ["chmod", "+x", "install-everything.sh"]
RUN ./install-everything.sh

# build client logging app
RUN ["chmod", "+x", "build-run-client.sh"]

# build and run the app
CMD ["./build-run-client.sh"]

