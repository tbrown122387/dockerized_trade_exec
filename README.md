# Interactive Brokers Gateway in Docker 

## Overview

This repository complements the functionality of [my other repo.](github.com/tbrown122387/dockerized_logger) Unlike the other repo, which only logs data, this one does not log any data, and instead submits orders to Interactive Brokers. 

## Quick start guide

On a host machine that you have shell access to (remote or local), clone this repo, change the config files, then type 

```docker-compose up``` 

or 

```docker-compose up -d``` 

if you want to run everything in the background. Every time this starts up, it will trigger a 2FA popup on your cell phone. 

Depending on how you installed Docker and Docker-Compose, you might need to add a `sudo` in front of those commands above. The first time you do this it will take a while--that is because it is building the entire Docker image. The good news is that the second time around everything is much quicker.

### Config Files

These are what you need to change before you `docker-compose up` everything. The config files are the ones with sensitive information, so be sure not to publicly share your edited version of this code. Or if you do, exclude these files. 

They are
 
1. your Interactive Brokers user id and password are going to be stored in `.env` 

2. the symbols you are interested in tracking (at the moment this is futures only!) are in `dockerized_logger/trade_app/ib_client/IBJts/samples/Cpp/TestCppClient/tickers.txt`


## Running it every day

If you want that to run every day automatically on a Linux host, you can make it a `cron` job. This is what I do. I'm sure there are analog tools for other OSs. 

To do that, type `sudo crontab -e`, and then add the following lines: 

```
# start up gateway through ibc every weekday at 8:30AM EST 
30 8 * * 1-5 cd ~/dockerized_logger && /usr/local/bin/docker-compose up

# nightly reboot at 4:05AM
0 4   *   *   *    /sbin/shutdown -r +5
30 4 * * 1-5 cd ~/dockerized_logger && /usr/local/bin/docker-compose down
```

## Visual Assessment of IB Gateway

If you need to watch live performance of IB Gateway, do the following after you've typed `docker-compose up`

1. Type `sudo docker ps -a` to get the container ID of `dockerized_trade_exec_tws`. Let's say that ended up being `42523a32b79b`

2. Type `docker exec -it 42523a32b79b /bin/bash` to run a terminal inside the image.

3. After the prompt changes, type `mkdir ~/.vnc && echo "mylongpassword" > ~/.vnc/passwd && x11vnc -passwd mylongpassword -display ":99" -forever -rfbport 5900`

4. Open up VNC viewer, or some similar program, log into `127.0.0.1:5902` and use the password `mylongpassword`


### Tips

The following mistakes don't really show up in the logs, so be careful:

1. using the wrong username/password, 

2. using the account or paper/live setting your data subscription is not associated with

3. specifying paper/live and forgetting to change `IB_GATEWAY_URLPORT` in `docker-compose.yml`.


