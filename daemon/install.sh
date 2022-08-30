#! /usr/bin/sh

pip install -r ./requirements.txt
sudo cp ./ups-daemon.conf /etc/
chmod +x ./ups-daemon.py
sudo cp ./ups-daemon.py /usr/local/sbin/
sudo cp ./ups-daemon.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now ups-daemon

