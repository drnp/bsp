<?php
/**
 * Interfaces of Bsp\Packet
 */
namespace Bsp;
interface IPacket
{
    public function pack($obj);
    public function unpack($data);
}
