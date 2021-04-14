#!/bin/bash
bash /home/snouri/Downloads/mynetworkproject/mynetworkproject/templatetags/bitsimulator-0.9.4+/tests/PureFloodingRoutingTest/firstRec.sh $(cat /home/snouri/Downloads/mynetworkproject/mynetworkproject/templatetags/bitsimulator-0.9.4+/tests/PureFloodingRoutingTest/events.log|grep ^0|head -1|cut -d' ' -f2)
