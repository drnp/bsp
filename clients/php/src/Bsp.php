<?php
/*
 * bsp.php
 *
 * Copyright (C) 2014 - Dr.NP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
 /**
  * BSP PHP Client
  * 
  * @package bsp::client::php
  * @author Dr.NP <np@bsgroup.org>
  * @update 06/12/2014
  * @changelog 
  *     [06/12/2014] - Creation
  */
namespace Bsp;

require('ISocket.php');
require('IPacket.php');
require('ICompressor.php');
require('Socket/Native.php');
require('Socket/Wrapper.php');
require('Packet/Amf.php');
require('Packet/Json.php');
require('Packet/Msgpack.php');
require('Packet/Native.php');
require('Compressor/Deflate.php');
require('Compressor/Lz4.php');
require('Compressor/Snappy.php');

class Client
{
    const SOCKET_DRIVER_NATIVE      = 0;
    const SOCKET_DRIVER_WRAPPER     = 1;
    const INET_TYPE_ANY             = 0;
    const INET_TYPE_IPV4            = 1;
    const INET_TYPE_IPV6            = 2;
    const INET_TYPE_LOCAL           = 3;
    const SOCK_TYPE_ANY             = 0;
    const SOCK_TYPE_TCP             = 1;
    const SOCK_TYPE_UDP             = 2;
    const DATA_TYPE_STREAM          = 0;
    const DATA_TYPE_PACKET          = 1;
    const SERIALIZE_TYPE_NATIVE     = 0;
    const SERIALIZE_TYPE_JSON       = 1;
    const SERIALIZE_TYPE_MSGPACK    = 2;
    const SERIALIZE_TYPE_AMF        = 3;
    const COMPRESS_TYPE_NONE        = 0;
    const COMPRESS_TYPE_DEFLATE     = 1;
    const COMPRESS_TYPE_LZ4         = 2;
    const COMPRESS_TYPE_SNAPPY      = 3;
    const PACKET_TYPE_REP           = 0;
    const PACKET_TYPE_RAW           = 1;
    const PACKET_TYPE_OBJ           = 2;
    const PACKET_TYPE_CMD           = 3;
    const PACKET_TYPE_HEARTBEAT     = 7;
    
    private $net;
    private $driver;
    private $host;
    private $port;
    private $inet;
    private $sock;
    private $data_type;
    private $serialize_type;
    private $compress_type;
    
    private $serializer;
    private $compressor;
    
    public function __construct($params = array())
    {
        $this->net = null;
        $this->driver = self::SOCKET_DRIVER_WRAPPER;
        $this->host = null;
        $this->port = 0;
        $this->inet = self::INET_TYPE_ANY;
        $this->sock = self::SOCK_TYPE_ANY;
        $this->data_type = self::DATA_TYPE_STREAM;
        $this->serialize_type = self::SERIALIZE_TYPE_JSON;
        $this->compress_type = self::COMPRESS_TYPE_NONE;
        
        $this->serializer = array();
        $this->compressor = array();
        
        if (isset($params['host']))
        {
            $this->host = \trim($params['host']);
        }
        if (isset($params['port']))
        {
            $this->port = \intval($params['port']);
        }
        if (isset($params['inet']))
        {
            if (\in_array($params['inet'], array(self::INET_TYPE_IPV4, self::INET_TYPE_IPV6, self::INET_TYPE_LOCAL)))
            {
                $this->inet = \intval($params['inet']);
            }
        }
        if (isset($params['sock']))
        {
            if (\in_array($params['sock'], array(self::SOCK_TYPE_TCP, self::SOCK_TYPE_UDP)))
            {
                $this->sock = \intval($params['sock']);
            }
        }
        if (isset($params['data']) && $params['data'] == self::DATA_TYPE_PACKET)
        {
            $this->data_type = self::DATA_TYPE_PACKET;
        }
        if (isset($params['serializer']))
        {
            if (\in_array($params['serializer'], array(self::SERIALIZE_TYPE_AMF, self::SERIALIZE_TYPE_MSGPACK, self::SERIALIZE_TYPE_NATIVE)))
            {
                $this->serialize_type = \intval($params['serializer']);
            }
        }
        if (isset($params['compressor']))
        {
            if (\in_array($params['compressor'], array(self::COMPRESS_TYPE_DEFLATE, self::COMPRESS_TYPE_LZ4, self::COMPRESS_TYPE_SNAPPY)))
            {
                $this->compress_type = \intval($params['compressor']);
            }
        }
        if (isset($params['driver']) && $params['driver'] == self::SOCKET_DRIVER_NATIVE)
        {
            $this->driver = self::SOCKET_DRIVER_NATIVE;
        }
        
        switch ($this->driver)
        {
            case self::SOCKET_DRIVER_NATIVE : 
                $domain = (self::INET_TYPE_IPV6 == $this->inet) ? \AF_INET6 : \AF_INET;
                $type = (self::SOCK_TYPE_UDP == $this->sock) ? \SOCK_DGRAM : \SOCK_STREAM;
                $protocol = (self::SOCK_TYPE_UDP == $this->sock) ? \SOL_UDP : \SOL_TCP;
                $this->net = new \Bsp\Socket\Native($domain, $type, $protocol);
                break;
            case self::SOCKET_DRIVER_WRAPPER : 
            default : 
                $protocol = (self::SOCK_TYPE_UDP == $this->sock) ? 'udp' : 'tcp';
                $this->net = new \Bsp\Socket\Wrapper($protocol);
                break;
        }
        
        // Serializer & Compressor
        $this->serializer[self::SERIALIZE_TYPE_AMF] = new \Bsp\Packet\Amf();
        $this->serializer[self::SERIALIZE_TYPE_JSON] = new \Bsp\Packet\Json();
        $this->serializer[self::SERIALIZE_TYPE_MSGPACK] = new \Bsp\Packet\Msgpack();
        $this->serializer[self::SERIALIZE_TYPE_NATIVE] = new \Bsp\Packet\Native();
        $this->compressor[self::COMPRESS_TYPE_DEFLATE] = new \Bsp\Compressor\Deflate();
        $this->compressor[self::COMPRESS_TYPE_LZ4] = new \Bsp\Compressor\Lz4();
        $this->compressor[self::COMPRESS_TYPE_SNAPPY] = new \Bsp\Compressor\Snappy();
        return;
    }
    
    public function __destruct()
    {
        if ($this->net)
        {
            $this->net->close();
        }
        
        return;
    }
    
    private function _hdr($type)
    {
        $hdr = (($type & 0b111) << 5) | 
               (($this->serialize_type & 0b111) << 2) | 
               (($this->compress_type & 0b11));
        
        return chr($hdr);
    }
    
    private function _len($len)
    {
        $ret = '';
        // Vint
        if (0 == $len >> 7)
        {
            // 1 bytes
            $ret = \chr($len & 0x7F);
        }
        elseif (0 == $len >> 14)
        {
            // 2 bytes
            $ret = \chr((($len >> 7) & 0x7F) | 0x80) . 
                   \chr($len & 0x7F);
        }
        elseif (0 == $len >> 21)
        {
            // 3 bytes
            $ret = \chr((($len >> 14) & 0x7F) | 0x80) . 
                   \chr((($len >> 7) & 0x7F) | 0x80) . 
                   \chr($len & 0x7F);
        }
        elseif (0 == $len >> 28)
        {
            // 4 bytes
            $ret = \chr((($len >> 21) & 0x7F) | 0x80) . 
                   \chr((($len >> 14) & 0x7F) | 0x80) . 
                   \chr((($len >> 7) & 0x7F) | 0x80) . 
                   \chr($len & 0x7F);
        }
        elseif (0 == $len >> 35)
        {
            // 5 bytes
            $ret = \chr((($len >> 28) & 0x7F) | 0x80) . 
                   \chr((($len >> 21) & 0x7F) | 0x80) . 
                   \chr((($len >> 14) & 0x7F) | 0x80) . 
                   \chr((($len >> 7) & 0x7F) | 0x80) . 
                   \chr($len & 0x7F);
        }
        elseif (0 == $len >> 42)
        {
            // 6 bytes
            $ret = \chr((($len >> 35) & 0x7F) | 0x80) . 
                   \chr((($len >> 28) & 0x7F) | 0x80) . 
                   \chr((($len >> 21) & 0x7F) | 0x80) . 
                   \chr((($len >> 14) & 0x7F) | 0x80) . 
                   \chr((($len >> 7) & 0x7F) | 0x80) . 
                   \chr($len & 0x7F);
        }
        elseif (0 == $len >> 49)
        {
            // 7 bytes
            $ret = \chr((($len >> 42) & 0x7F) | 0x80) . 
                   \chr((($len >> 35) & 0x7F) | 0x80) . 
                   \chr((($len >> 28) & 0x7F) | 0x80) . 
                   \chr((($len >> 21) & 0x7F) | 0x80) . 
                   \chr((($len >> 14) & 0x7F) | 0x80) . 
                   \chr((($len >> 7) & 0x7F) | 0x80) . 
                   \chr($len & 0x7F);
        }
        elseif (0 == $len >> 56)
        {
            // 8 bytes
            $ret = \chr((($len >> 49) & 0x7F) | 0x80) . 
                   \chr((($len >> 42) & 0x7F) | 0x80) . 
                   \chr((($len >> 35) & 0x7F) | 0x80) . 
                   \chr((($len >> 28) & 0x7F) | 0x80) . 
                   \chr((($len >> 21) & 0x7F) | 0x80) . 
                   \chr((($len >> 14) & 0x7F) | 0x80) . 
                   \chr((($len >> 7) & 0x7F) | 0x80) . 
                   \chr($len & 0x7F);
        }
        else
        {
            // 9 bytes
            $ret = \chr((($len >> 56) & 0x7F) | 0x80) . 
                   \chr((($len >> 49) & 0x7F) | 0x80) . 
                   \chr((($len >> 42) & 0x7F) | 0x80) . 
                   \chr((($len >> 35) & 0x7F) | 0x80) . 
                   \chr((($len >> 28) & 0x7F) | 0x80) . 
                   \chr((($len >> 21) & 0x7F) | 0x80) . 
                   \chr((($len >> 14) & 0x7F) | 0x80) . 
                   \chr((($len >> 7) & 0x7F) | 0x80) . 
                   \chr($len & 0x7F);
        }
        
        return $ret;
    }
    
    private function _parse_hdr($hdr)
    {
        $ret = array(
            'type' => self::PACKET_TYPE_REP, 
            'serialize_type' => $this->serialize_type, 
            'compress_type' => $this->compress_type
        );
        
        if ($hdr !== false)
        {
            if (\is_string($hdr))
            {
                $code = \ord($hdr[0]);
            }
            else
            {
                $code = (\intval($hdr)) & 0xFF;
            }
            
            $ret['type'] = ($code >> 5);
            $ret['serialize_type'] = ($code >> 2) & 0b111;
            $ret['commpress_type'] = $code & 0b11;
        }
        
        return $ret;
    }
    
    private function _parse_len($stream)
    {
        $ret = 0;
        if (\is_string($stream))
        {
            $ret = \ord($stream[0]) & 0x7F;
            if (0 == \ord($stream[0]) | 0x80)
            {
                return $ret;
            }
            $ret = $ret << 7 | (\ord($stream[1]) & 0x7F);
            if (0 == \ord($stream[1]) & 0x80)
            {
                return $ret;
            }
            $ret = $ret << 7 | (\ord($stream[2]) & 0x7F);
            if (0 == \ord($stream[2]) & 0x80)
            {
                return $ret;
            }
            $ret = $ret << 7 | (\ord($stream[3]) & 0x7F);
            if (0 == \ord($stream[3]) & 0x80)
            {
                return $ret;
            }
            $ret = $ret << 7 | (\ord($stream[4]) & 0x7F);
            if (0 == \ord($stream[4]) & 0x80)
            {
                return $ret;
            }
            $ret = $ret << 7 | (\ord($stream[5]) & 0x7F);
            if (0 == \ord($stream[5]) & 0x80)
            {
                return $ret;
            }
            $ret = $ret << 7 | (\ord($stream[6]) & 0x7F);
            if (0 == \ord($stream[6]) & 0x80)
            {
                return $ret;
            }
            $ret = $ret << 7 | (\ord($stream[7]) & 0x7F);
            if (0 == \ord($stream[7]) & 0x80)
            {
                return $ret;
            }
            $ret = $ret << 8 | (\ord($stream[8]) & 0xFF);
        }
        
        return $ret;
    }
    
    private function _cmd($cmd)
    {
        $ret = \pack('N', $cmd);
        
        return $ret;
    }
    
    public function connect()
    {
        if (!$this->host || !$this->port)
        {
            die('Cannot create socket connection over empty host or port');
        }
        
        if ($this->net)
        {
            $this->net->open($this->host, $this->port);
        }
        
        if (self::DATA_TYPE_PACKET == $this->data_type)
        {
            // Send report packet
            $this->net->send($this->_hdr(self::PACKET_TYPE_REP));
            $this->recv();
        }
        
        return;
    }
    
    public function disconnect()
    {
        if ($this->net)
        {
            $this->net->close();
        }
        
        return;
    }
    
    public function send()
    {
        if (!$this->net)
        {
            return 0;
        }
        
        $args = \func_get_args();
        if (\is_string($args[0]))
        {
            // Raw data
            $data = $args[0] . '';
            if (self::DATA_TYPE_STREAM == $this->data_type)
            {
                // Send original data immediatelly
                return $this->net->send($data);
            }
            else
            {
                if (self::COMPRESS_TYPE_NONE != $this->compress_type)
                {
                    $data = $this->compressor[$this->compress_type]->compress($data);
                }
                return $this->net->send($this->_hdr(self::PACKET_TYPE_RAW) . $this->_len(\strlen($data)) . $data);
            }
        }
        elseif (\is_array($args[0]) && self::DATA_TYPE_PACKET == $this->data_type)
        {
            // Object
            $obj = $args[0];
            $data = $this->serializer[$this->serialize_type]->pack($obj);
            if (self::COMPRESS_TYPE_NONE != $this->compress_type)
            {
                $data = $this->compressor[$this->compress_type]->compress($data);
            }
            
            return $this->net->send($this->_hdr(self::PACKET_TYPE_OBJ) . $this->_len(\strlen($data)) . $data);
        }
        elseif (\is_int($args[0]) && \is_array($args[1]) && self::DATA_TYPE_PACKET == $this->data_type)
        {
            // Cmd
            $cmd = $args[0];
            $params = $args[1];
            $data = $this->_cmd($cmd) . $this->serializer[$this->serialize_type]->pack($params);
            if (self::COMPRESS_TYPE_NONE != $this->compress_type)
            {
                $data = $this->compressor[$this->compress_type]->compress($data);
            }
            
            return $this->net->send($this->_hdr(self::PACKET_TYPE_CMD) . $this->_len(\strlen($data)) . $data);
        }
        
        return;
    }
    
    public function recv()
    {
        if (!$this->net)
        {
            return '';
        }
        $data = $this->net->recv();
        if (self::DATA_TYPE_STREAM == $this->data_type)
        {
            return $data;
        }
        else
        {
            $offset = 0;
            $llen = 4;
            $ret = array();
            while (true)
            {
                $stream = \substr($data, $offset);
                $hdr = $this->_parse_hdr($stream);
                $offset += 1;
                if (self::PACKET_TYPE_RAW == $hdr['type'] || 
                    self::PACKET_TYPE_OBJ == $hdr['type'] || 
                    self::PACKET_TYPE_CMD == $hdr['type'])
                {
                    // Parse data
                    $len = $this->_parse_len($stream);
                    $llen = (self::LENGTH_TYPE_32B == $this->length_type) ? 4 : 8;
                    if (\strlen($stream) >= 1 + $llen + $len)
                    {
                        $org_data = \substr($stream, 1 + $llen, $len);
                        if (self::COMPRESS_TYPE_NONE != $this->compress_type)
                        {
                            $org_data = $this->compressor[$this->compress_type]->decompress($org_data);
                        }
                        // Packet
                        switch ($hdr['type'])
                        {
                            case self::PACKET_TYPE_RAW : 
                                $ret[] = array('type' => 'raw', 'raw' => $org_data);
                                break;
                            case self::PACKET_TYPE_OBJ : 
                                $ret[] = array('type' => 'obj', 'obj' => $this->serializer[$this->serialize_type]->unpack($org_data));
                                break;
                            case self::PACKET_TYPE_CMD : 
                                $cmd = (\ord($org_data[0]) << 24) + 
                                        (\ord($org_data[1]) << 16) + 
                                        (\ord($org_data[2]) << 8) + 
                                        (\ord($org_data[3]));
                                $params = $this->serializer[$this->serialize_type]->unpack(\substr($org_data, 4));
                                $ret[] = array('type' => 'cmd', 'cmd' => $cmd, 'params' => $params);
                                break;
                            default : 
                                // WTF
                                break;
                        }
                    }
                    
                    $offset += $llen + $len;
                }
                else
                {
                    // Ignore
                }
                
                if ($offset >= \strlen($data))
                {
                    break;
                }
            }
            
            return $ret;
        }
    }
}
