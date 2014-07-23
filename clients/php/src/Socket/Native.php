<?php
/*
 * Native.php
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
 * Native(sockets) network driver
 * 
 * @package bsp::client::php
 * @author Dr.NP <np@bsgroup.org>
 * @update 06/16/2014
 * @changelog 
 *      [06/16/2014] - Creation
 */

namespace Bsp\Socket;

class Native implements \Bsp\ISocket
{
    const READ_ONCE                 = 4096;
    const RETIRES                   = 6;
    
    private $sockfp;
    
    public function __construct($domain, $type, $protocol)
    {
        $this->sockfp = \socket_create($domain, $type, $protocol);
        return;
    }
    
    public function __destruct()
    {
        $this->sockfp = null;
        
        return;
    }
    
    public function open($host, $port)
    {
        if ($this->sockfp)
        {
            for ($n = 0; $n < self::RETIRES; $n ++)
            {
                if (\socket_connect($this->sockfp, $host, $port))
                {
                    break;
                }
            }
            //\socket_set_nonblock($this->sockfp);
        }
        
        return;
    }
    
    public function close()
    {
        if ($this->sockfp)
        {
            \socket_close($this->sockfp);
        }
        
        return;
    }
    
    public function send($data)
    {
        $sent = 0;
        $size = \strlen($data);
        if ($this->sockfp)
        {
            while (true)
            {
                $n = \socket_write($this->sockfp, \substr($data, $sent), $size - $sent);
                if (!$n)
                {
                    break;
                }
                $sent += $n;
                if ($sent >= $size)
                {
                    break;
                }
            }
        }
        
        return $sent;
    }
    
    public function recv()
    {
        $data = '';
        if ($this->sockfp)
        {
            while (true)
            {
                $read = \socket_read($this->sockfp, self::READ_ONCE);
                if (!$read)
                {
                    break;
                }
                $data .= $read;
                if (\strlen($read) < self::READ_ONCE)
                {
                    break;
                }
                else
                {
                    \socket_set_nonblock($this->sockfp);
                }
            }
        }
        \socket_set_block($this->sockfp);
        return $data;
    }
}
