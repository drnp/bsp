<?php
/*
 * Deflate.php
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
 * Compressor::Deflate
 * 
 * @package bsp::client::php
 * @author Dr.NP <np@bsgroup.org>
 * @update 06/17/2014
 * @changelog 
 *      [06/17/2014] - Creation
 */

namespace Bsp\Compressor;

class Deflate implements \Bsp\ICompressor
{
    public function __construct()
    {
        return;
    }
    
    public function __destruct()
    {
        return;
    }
    
    public function compress($input)
    {
        $output = $input;
        
        return $output;
    }
    
    public function decompress($input)
    {
        $output = $input;
        
        return $output;
    }
}
