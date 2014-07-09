<?php
/**
 * BSP PHP Client test
 */
require("src/Bsp.php");

$config = array(
    'host'      => '192.168.100.251', 
    'port'      => 9172, 
    'data'      => \Bsp\Client::DATA_TYPE_PACKET, 
    'driver'    => \Bsp\Client::SOCKET_DRIVER_WRAPPER, 
    'inet'      => \Bsp\Client::INET_TYPE_IPV4, 
    'sock'      => \Bsp\Client::SOCK_TYPE_TCP, 
    'serializer'=> \Bsp\Client::SERIALIZE_TYPE_NATIVE
);

$bsp = new \Bsp\Client($config);
$bsp->connect();
\sleep(1);
$bsp->send(9531, array());
$data = $bsp->recv();
$bsp->disconnect();
