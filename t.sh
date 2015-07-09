#!/bin/bash
set -euo pipefail
IFS=$'\n\t'

OLD={{__IP__}}
NEW=$(docker-machine ip)

echo the ip to replace is $NEW

items="docker-compose.yml"

for i in $items; do 
	sed -i '.bak' "s/$OLD/$NEW/g" docker-compose.yml.tpl > docker-compose.yml
done 

