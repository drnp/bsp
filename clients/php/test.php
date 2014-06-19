<?php
/**
 * BSP PHP Client test
 */
require("src/Bsp.php");

$config = array(
    'host'      => 'localhost', 
    'port'      => 8254, 
    'data'      => \Bsp\Client::DATA_TYPE_PACKET, 
    'driver'    => \Bsp\Client::SOCKET_DRIVER_WRAPPER, 
    'inet'      => \Bsp\Client::INET_TYPE_IPV4, 
    'sock'      => \Bsp\Client::SOCK_TYPE_TCP, 
    'serializer'=> \Bsp\Client::SERIALIZE_TYPE_NATIVE
);

$bsp = new \Bsp\Client($config);
$bsp->connect();
\sleep(1);
$bsp->send(1000, array('interval' => 3, 'host' => 'www.baidu.com', 'uri' => '/'));
$data = $bsp->recv();
$bsp->disconnect();