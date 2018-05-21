#!/bin/bash

if [[ $1 == "" ]]; then
  echo "Usage : $0 [boxid|all|active]"
fi

source "../venice_fs/usr/bin/peoplebox-init"
source "../venice_fs/.ssh/ip.sh" $1

for indice in ${!nomi[*]}
do
  nome="${nomi[$indice]}"
  indirizzo="${indirizzi[$indice]}"
  camip="${cameraip[$indice]}"
  if [[ $nome = *"raspi"* ]]; then
    nomeuser=pi
    password=raspberry
  else
    nomeuser=nvidia
    password=nvidia
  fi
  echo "****) Milking frames from $nomeuser@$nome"

  # sync dump_frame and setup
  ssh "${nomeuser}"@"${nome}" scp -P 44450 venice@osmino.bo.infn.it:dump_frame Codice/peoplebox/
  ssh "${nomeuser}"@"${nome}" chmod 755 Codice/peoplebox/dump_frame
  ssh "${nomeuser}"@"${nome}" mkdir -p Codice/peoplebox/frames

  for i in {1..30}; do
    echo "---- Milking $i"
    # launch dump_frame with proper command line
    ssh "${nomeuser}"@"${nome}" "cd Codice/peoplebox; ./dump_frame 50 rtsp://physycom:ohnounapassWORD55@${camip}:554/rtpstream/config1=u" >> milk.log

    # rsync output
    local_user=nvidia
    ssh "${nomeuser}"@"${nome}" "ssh -p $unibo_port -i \$HOME/.ssh/id_rsa $unibo_user@$unibo_host mkdir -p milk/$nome/frames$i"
    ssh "${nomeuser}"@"${nome}" "$RSYNC --remove-source-files ${RSYNC_FLAGS} -e \"ssh -p $unibo_port -i \$HOME/.ssh/id_rsa\" Codice/peoplebox/frames/*.jpg $unibo_user@$unibo_host:milk/$nome/frames$i"
  done
done
