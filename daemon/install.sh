#! /usr/bin/sh

if [ "$1" != "update" ]; then
  pip install -r ./requirements.txt
else
  sudo systemctl disable --now ups-daemon
fi

sudo cp ./ups-daemon.conf /etc/
chmod +x ./ups-daemon
sudo cp ./ups-daemon /usr/local/sbin/
sudo cp ./ups-daemon.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now ups-daemon
