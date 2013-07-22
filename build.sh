#!/bin/bash
make clean all
sudo cp modules/gfilter.so /usr/lib/php/modules/
sudo service httpd restart
