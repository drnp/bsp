<?php
/**
 * Interface of Bsp\Socket
 */
namespace Bsp;
interface ISocket
{
    public function open($host, $port);
    public function close();
    public function send($data);
    public function recv();
}
