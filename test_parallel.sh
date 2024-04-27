#!/bin/bash
# script.sh

# Caminho para o executável do cliente
CLIENT="./client"

# Envia várias tarefas para o servidor
$CLIENT execute 5000 -u "sleep 5" &
$CLIENT execute 3000 -u "sleep 3" &
$CLIENT execute 4000 -u "sleep 4" &
$CLIENT execute 2000 -u "sleep 2" &
$CLIENT execute 1000 -u "sleep 1" &
$CLIENT execute 1000 -u "echo 'Hello World'" &

$CLIENT execute 5000 -p "echo 'Hello New York\nHello Seattle\nHello Berlin\nHello New York' | grep 'New' | sort | uniq" &
$CLIENT execute 1000 -p "yes 'a' | head -n 3" &

# Aguarda todos os processos em background terminarem
wait