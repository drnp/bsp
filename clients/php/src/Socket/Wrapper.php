<?php
/*
 * Wrapper.php
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
 * Wrapper(stream) network driver
 * 
 * @package bsp::client::php
 * @author Dr.NP <np@bsgroup.org>
 * @update 06/16/2014
 * @changelog 
 *      [06/16/2014] - Creation
 */

namespace Bsp\Socket;

class Wrapper implements \Bsp\ISocket
{
    const READ_ONCE                 = 4096;

    private $protocol_prefix;
    private $sockfp;
    
    public function __construct($protocol)
    {
        $this->sockfp = null;
        $this->protocol_prefix = \trim($protocol);
        
        return;
    }
    
    public function __destruct()
    {
        $this->sockfp = null;
        
        return;
    }
    
    public function open($host, $port)
    {
        $remote = $this->protocol_prefix . '://' . $host . ':' . $port;
        $errno = 0;
        $errstr = '';
        $this->sockfp = \stream_socket_client($remote, $errno, $errstr);
        if ($this->sockfp)
        {
            \stream_set_blocking($this->sockfp, 0);
        }
        
        return;
    }
    
    public function close()
    {
        if ($this->sockfp)
        {
            \fclose($this->sockfp);
            $this->sockfp = null;
        }
        
        return;
    }
    
    public function send($data)
    {
        if ($this->sockfp)
        {
            return \fwrite($this->sockfp, $data);
        }
        
        return 0;
    }
    
    public function recv()
    {
        $data = '';
        if ($this->sockfp)
        {
            while (!\feof($this->sockfp))
            {
                $read = \fread($this->sockfp, self::READ_ONCE);
                if (!$read)
                {
                    break;
                }
                else
                {
                    $data .= $read;
                }
            }
        }
        
        return $data;
    }
}
