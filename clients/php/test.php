<?php
/**
 * BSP PHP Client test
 */
require("src/Bsp.php");

$config = array(
    'host'      => '192.168.100.251', 
    'port'      => 9173, 
    'data'      => \Bsp\Client::DATA_TYPE_PACKET, 
    'driver'    => \Bsp\Client::SOCKET_DRIVER_WRAPPER, 
    'inet'      => \Bsp\Client::INET_TYPE_IPV4, 
    'sock'      => \Bsp\Client::SOCK_TYPE_TCP, 
    'serializer'=> \Bsp\Client::SERIALIZE_TYPE_JSON
);

$bsp = new \Bsp\Client($config);
$bsp->connect();
//\sleep(1);
/*
$data = array(
    'pids'  => array(1), 
    'msg'   => array(
        'msg'   => array(
            'cityList'  => array(
                'kid'       => 30004, 
                'city_id'   => 7, 
                'union'     => array(
                    'union_id'      => 1112, 
                    'union_name'    => '五国', 
                    'union_level'   => 1, 
                    'flag_no'       => 1, 
                    'flag_name'     => '宋JJ'
                ), 
                'lastTime'  => 0
            ), 
            'country'   => 3, 
            'points'    => 160, 
            'rank'      => array(
                array(
                    'points'    => 160, 
                    'union'     => array(
                        'union_id'      => 1112, 
                        'union_name'    => '五国', 
                        'union_level'   => 1, 
                        'flag_no'       => 1, 
                        'flag_name'     => '宋JJ'
                    )
                ), 
                array(
                    'points'    => 90, 
                    'union'     => array(
                        'union_id'      => 1115, 
                        'union_name'    => '孙武', 
                        'union_level'   => 1, 
                        'flag_no'       => 1, 
                        'flag_name'     => '孙'
                    )
                )
            ), 
            'type'      => 1
        )
    )
);

$bsp->send(9529, $data);
$data = array(
    'timeout'       => 29, 
    'msg'           => 'msg%5BholdTime%5D=1405064544&msg%5Bkid%5D=30004&msg%5Bunion_id%5D=1112&msg%5BisCountry%5D=1&msg%5BcityType%5D=2&msg%5Bcity_id%5D=7&msg%5BeventId%5D=1'
);
$bsp->send(9527, $data);
$recv = $bsp->recv();
 */
$bsp->send(99999, array());
$recv = $bsp->recv();
\var_dump($recv);

$bsp->disconnect();
