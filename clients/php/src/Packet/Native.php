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
 * Serializer::Native
 * 
 * @package bsp::client::php
 * @author Dr.NP <np@bsgroup.org>
 * @update 06/17/2014
 * @changelog 
 *      [06/17/2014] - Creation
 */

namespace Bsp\Packet;

class Native implements \Bsp\IPacket
{
    private $endian;
    private $str;
    private $obj;
    private $offset;
    
    const BSP_VAL_INT8              = 0x1;
    const BSP_VAL_INT16             = 0x2;
    const BSP_VAL_INT32             = 0x3;
    const BSP_VAL_INT64             = 0x4;
    const BSP_VAL_FLOAT             = 0x11;
    const BSP_VAL_DOUBLE            = 0x12;
    const BSP_VAL_STRING            = 0x21;
    const BSP_VAL_OBJECT            = 0x51;
    const BSP_VAL_OBJECT_END        = 0x52;
    const BSP_VAL_ARRAY             = 0x61;
    const BSP_VAL_ARRAY_END         = 0x62;
    const BSP_VAL_NULL              = 0x7F;
    
    const BSP_ENDIAN_LITTLE         = 0x0;
    const BSP_ENDIAN_BIG            = 0x1;
    
    public function __construct()
    {
        $tmp = \pack('l', 1);
        if (1 != \ord($tmp[3]))
        {
            $this->endian = self::BSP_ENDIAN_LITTLE;
        }
        else
        {
            $this->endian = self::BSP_ENDIAN_BIG;
        }
        
        return;
    }
    
    public function __destruct()
    {
        return;
    }
    
    private function _type($val)
    {
        if (\is_int($val))
        {
            // Integer
            $r_val = \intval($val);
            if (($r_val >> 32) > 0)
            {
                return self::BSP_VAL_INT64;
            }
            elseif (($r_val >> 16) > 0)
            {
                return self::BSP_VAL_INT32;
            }
            elseif (($r_val >> 8) > 0)
            {
                return self::BSP_VAL_INT16;
            }
            else
            {
                return self::BSP_VAL_INT8;
            }
        }
        elseif (\is_numeric($val))
        {
            // Double
            return self::BSP_VAL_DOUBLE;
        }
        elseif (\is_string($val))
        {
            return self::BSP_VAL_STRING;
        }
        elseif (\is_null($val))
        {
            return self::BSP_VAL_NULL;
        }
        elseif (\is_array($val))
        {
            // Array or Object [Hash]
            $tkeys = \array_keys($val);
            \sort($tkeys, SORT_NUMERIC);
            \reset($tkeys);
            while ($tval = \current($tkeys))
            {
                if (\key($tkeys) !== $tval)
                {
                    // Not array
                    return self::BSP_VAL_OBJECT;
                }
                \next($tkeys);
            }
            
            return self::BSP_VAL_ARRAY;
        }
        else
        {
            // Unknown type
            return -1;
        }
    }
    
    private function _get_int8()
    {
        $val = 0;
        if ($this->offset <= \strlen($this->str) - 1)
        {
            $val = \ord($this->str[$this->offset]);
            if ($val > 0x7F)
            {
                $val -= 0x100;
            }
        }
        $this->offset += 1;
        
        return $val;
    }
    
    private function _get_int16()
    {
        $val = 0;
        if ($this->offset <= \strlen($this->str) - 2)
        {
            $val = (\ord($this->str[$this->offset]) << 8) +
                    \ord($this->str[$this->offset + 1]);
            if ($val > 0x7FFF)
            {
                $val -= 0x10000;
            }
        }
        $this->offset += 2;
        
        return $val;
    }
    
    private function _get_int32()
    {
        $val = 0;
        if ($this->offset <= \strlen($this->str) - 4)
        {
            $val = (\ord($this->str[$this->offset]) << 24) +
                    (\ord($this->str[$this->offset + 1]) << 16) +
                    (\ord($this->str[$this->offset + 2]) << 8) +
                    \ord($this->str[$this->offset + 3]);
            if ($val > 0x7FFFFFFF)
            {
                $val -= 0x100000000;
            }
        }
        $this->offset += 4;
        
        return $val;
    }
    
    private function _get_int64()
    {
        $val = 0;
        if ($this->offset <= \strlen($this->str) - 8)
        {
            if (8 == PHP_INT_SIZE)
            {
                // 64 bit
                $val = (\ord($this->str[$this->offset]) << 56) +
                        (\ord($this->str[$this->offset + 1]) << 48) +
                        (\ord($this->str[$this->offset + 2]) << 40) +
                        (\ord($this->str[$this->offset + 3]) << 32) +
                        (\ord($this->str[$this->offset + 4]) << 24) +
                        (\ord($this->str[$this->offset + 5]) << 16) +
                        (\ord($this->str[$this->offset + 6]) << 8) +
                        \ord($this->str[$this->offset + 7]);
            }
            else
            {
                // 32 bit
                $val = (\ord($this->str[$this->offset + 4]) << 24) +
                        (\ord($this->str[$this->offset + 5]) << 16) +
                        (\ord($this->str[$this->offset + 6]) << 8) +
                        \ord($this->str[$this->offset + 7]);
                if ($val > 0x7FFFFFFF)
                {
                    $val -= 0x100000000;
                }
            }
        }
        $this->offset += 8;
        
        return $val;
    }
    
    private function _get_float()
    {
        //$val = 0.0;
        if ($this->offset <= \strlen($this->str) - 4)
        {
            $tmp = $this->str[$this->offset] .
                    $this->str[$this->offset + 1] .
                    $this->str[$this->offset + 2] .
                    $this->str[$this->offset + 3];
            $arr = unpack('fval', $tmp);
        }
        $this->offset += 4;
        
        return $arr['val'];
    }
    
    private function _get_double()
    {
        //$val = 0.0;
        if ($this->offset <= \strlen($this->str) - 8)
        {
            $tmp = $this->str[$this->offset] .
                    $this->str[$this->offset + 1] .
                    $this->str[$this->offset + 2] .
                    $this->str[$this->offset + 3] .
                    $this->str[$this->offset + 4] .
                    $this->str[$this->offset + 5] .
                    $this->str[$this->offset + 6] .
                    $this->str[$this->offset + 7];
            $arr = \unpack('dval', $tmp);
        }
        $this->offset += 8;
        
        return $arr['val'];
    }
    
    private function _get_string()
    {
        $val = '';
        if (\Bsp\Client::LENGTH_TYPE_32B == $this->length_type && $this->offset <= \strlen($this->str) - 4)
        {
            $len = (\ord($this->str[$this->offset]) << 24) +
                    (\ord($this->str[$this->offset + 1]) << 16) +
                    (\ord($this->str[$this->offset + 2]) << 8) +
                    \ord($this->str[$this->offset + 3]);
            $this->offset += 4;
        }
        elseif ($this->offset <= \strlen($this->str) - 8)
        {
            $len = (\ord($this->str[$this->offset + 4]) << 24) +
                    (\ord($this->str[$this->offset + 5]) << 16) +
                    (\ord($this->str[$this->offset + 6]) << 8) +
                    \ord($this->str[$this->offset + 7]);
            $this->offset += 8;
        }
        else
        {
            $this->offset = \strlen($this->str) - 1;
            
            return '';
        }
        
        if ($this->offset <= \strlen($this->str) - $len)
        {
            $val = \substr($this->str, $this->offset, $len);
        }
        $this->offset += $len;
        
        return $val;
    }
    
    private function _pack_object($obj, $is_array = false)
    {
        if (!$obj || !\is_array($obj))
        {
            return;
        }
        
        \reset($obj);
        while (($curr = \current($obj)) !== false)
        {
            if (!$is_array)
            {
                // Record key
                $key = \key($obj) . '';
                if (\Bsp\Client::LENGTH_TYPE_32B == $this->length_type)
                {
                    // int32
                    $this->str .= \pack('cN', self::BSP_VAL_STRING, \strlen($key));
                }
                else
                {
                    // int64
                    $this->str .= \pack('cNN', self::BSP_VAL_STRING, 0, \strlen($key));
                }
                
                $this->str .= $key;
            }
            
            // Then write data
            switch ($this->_type($curr))
            {
                case self::BSP_VAL_INT8 :
                    $this->str .= \pack('cc', self::BSP_VAL_INT8, $curr);
                    break;
                case self::BSP_VAL_INT16 :
                    $this->str .= \pack('cn', self::BSP_VAL_INT16, $curr);
                    break;
                case self::BSP_VAL_INT32 :
                    $this->str .= \pack('cN', self::BSP_VAL_INT32, $curr);
                    break;
                case self::BSP_VAL_INT64 :
                    $this->str .= \pack('cNN', self::BSP_VAL_INT64, $curr >> 32, $curr & 0xFFFFFFFF);
                    break;
                case self::BSP_VAL_FLOAT :
                case self::BSP_VAL_DOUBLE :
                    $this->str .= \pack('cd', self::BSP_VAL_DOUBLE, $curr);
                    break;
                case self::BSP_VAL_NULL :
                    $this->str .= \pack('c', self::BSP_VAL_NULL);
                    break;
                case self::BSP_VAL_STRING :
                    // Write length first
                    if (\Bsp\Client::LENGTH_TYPE_32B == $this->length_type)
                    {
                        // Int32
                        $this->str .= \pack('cN', self::BSP_VAL_STRING, \strlen($curr));
                    }
                    else
                    {
                        $this->str .= \pack('cNN', self::BSP_VAL_STRING, 0, \strlen($curr));
                    }
                    $this->str .= $curr;
                    break;
                case self::BSP_VAL_ARRAY :
                    $this->str .= \pack('c', self::BSP_VAL_ARRAY);
                    $this->_pack_object($curr, true);
                    $this->str .= \pack('c', self::BSP_VAL_ARRAY_END);
                    break;
                case self::BSP_VAL_OBJECT :
                    $this->str .= \pack('c', self::BSP_VAL_OBJECT);
                    $this->_pack_object($curr, false);
                    $this->str .= \pack('c', self::BSP_VAL_OBJECT_END);
                    break;
                default :
                    // Unknown ~~
            }
            \next($obj);
        }
        
        return;
    }
    
    private function _unpack_object($has_key = true)
    {
        $len = \strlen($this->str);
        $ret = array();
        $key = null;
        while ($this->offset < $len)
        {
            if ($has_key)
            {
                $type = $this->_get_int8();
                // Read key first
                if (self::BSP_VAL_STRING == $type)
                {
                    $key = $this->_get_string();
                }
                // Object ending
                elseif (self::BSP_VAL_OBJECT_END == $type)
                {
                    return $ret;
                }
            }
            
            // Read value
            $type = $this->_get_int8();
            switch ($type)
            {
                case self::BSP_VAL_INT8 :
                    $val = $this->_get_int8();
                    break;
                case self::BSP_VAL_INT16 :
                    $val = $this->_get_int16();
                    break;
                case self::BSP_VAL_INT32 :
                    $val = $this->_get_int32();
                    break;
                case self::BSP_VAL_INT64 :
                    $val = $this->_get_int64();
                    break;
                case self::BSP_VAL_FLOAT :
                    $val = $this->_get_float();
                    break;
                case self::BSP_VAL_DOUBLE :
                    $val = $this->_get_double();
                    break;
                case self::BSP_VAL_NULL :
                    $val = null;
                    break;
                case self::BSP_VAL_STRING :
                    $val = $this->_get_string();
                    break;
                case self::BSP_VAL_ARRAY :
                    $val = $this->_unpack_object(false);
                    break;
                case self::BSP_VAL_ARRAY_END :
                    return $ret;
                case self::BSP_VAL_OBJECT :
                    $val = $this->_unpack_object();
                    break;
                case self::BSP_VAL_OBJECT_END :
                    return $ret;
                default :
                    // ~~ Unknown data type
                    break;
            }
            ($key) ? $ret[$key] = $val : $ret[] = $val;
        }
        return $ret;
    }
    
    public function pack($obj)
    {
        if (!$obj || !\is_array($obj))
        {
            return '';
        }
        
        $this->obj = $obj;
        $this->str = '';
        $this->offset = 0;
        
        $this->_pack_object($this->obj);
        
        return $this->str;
    }
    
    public function unpack($data)
    {
        if (!$data || !\is_string($data) || 0 == \strlen($data))
        {
            return array();
        }
        
        $this->str = $data;
        $this->offset = 0;
        $this->obj = $this->_unpack_object();
        
        return $this->obj;
    }
}
