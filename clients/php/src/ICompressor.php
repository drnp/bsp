<?php
/**
 * Interfaces of Bsp\Compressor
 */
namespace Bsp;
interface ICompressor
{
    public function compress($input);
    public function decompress($input);
}
