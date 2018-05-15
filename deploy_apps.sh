#!/bin/bash

usage(){
  echo "Usage : $0 [new|del|upd] <device_tag>"
  exit 1
}

case $1 in
  "new"|"del"|"upd")
		if [[ $2 != "" ]]; then
			scp darktrack_$2 venice@osmino:$2/bin/darktrack.$1
			scp save_frame_$2 venice@osmino:$2/bin/save_frame.$1
		fi
		;;
	*)
		usage
		;;
esac
