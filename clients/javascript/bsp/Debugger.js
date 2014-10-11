/*
 * Debugger.js
 *
 * Copyright (C) 2012 - Dr.NP
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
 * Object dumper
 * 
 * @package bsp
 * @author Dr.NP <np@bsgroup.org>
 * @update 10/16/2012
 * @changelog 
 * 		[10/16/2012] - Creation
 */
 
function dump_object(o)
{
	var _dump_object = function(o) {
		var indent = arguments[1] ? parseInt(arguments[1]) : 0;
		var rtn = '';
		var idt;
		for (var key in o)
		{
			idt = '';
			for (var i = 0; i < indent; i ++)
			{
				idt += "\t";
			}
			
			switch (typeof(o[key]))
			{
				case 'string' : 
					console.debug(idt + '[' + key + '] => (String) : ' + o[key]);
					rtn += idt + '[' + key + '] => (String) : ' + o[key] + "\n";
					break;
				case 'number' : 
					console.debug(idt + '[' + key + '] => (Number) : ' + o[key]);
					rtn += idt + '[' + key + '] => (Number) : ' + o[key] + "\n";
					break;
				case 'boolean' : 
					console.debug(idt + '[' + key + '] => (Boolean) : ' + o[key]);
					rtn += idt + '[' + key + '] => (Boolean) : ' + o[key] + "\n";
					break;
				case 'object' : 
					if (Array.isArray(o[key]))
					{
						console.debug(idt + '[' + key + '] => (Array) : ');
						rtn += idt + '[' + key + '] => (Array) : ' + _dump_array(o[key], indent + 1);
					}
					else
					{
						console.debug(idt + '[' + key + '] => (Object) : ');
						rtn += idt + '[' + key + '] => (Object) : ' + _dump_object(o[key], indent + 1);
					}
					break;
				default : 
					console.debug(idt + '[' + key +'] => Unknown data type');
					rtn += idt + '[' + key + "] => Unknown data type\n";
					break;
			}
		}
		
		if (indent == 0)
		{
			console.debug('=============================================');
		}
		
		rtn += "\n";
		return rtn;
	}
	
	var _dump_array = function(o) {
		var indent = arguments[1] ? parseInt(arguments[1]) : 0;
		var rtn = '';
		var idt;
		for (var key = 0; key < o.length; key ++)
		{
			idt = '';
			for (var i = 0; i < indent; i ++)
			{
				idt += "\t";
			}
			
			switch (typeof(o[key]))
			{
				case 'string' : 
					console.debug(idt + '[' + key + '] => (String) : ' + o[key]);
					rtn += idt + '[' + key + '] => (String) : ' + o[key] + "\n";
					break;
				case 'number' : 
					console.debug(idt + '[' + key + '] => (Number) : ' + o[key]);
					rtn += idt + '[' + key + '] => (Number) : ' + o[key] + "\n";
					break;
				case 'object' : 
					if (Array.isArray(o[key]))
					{
						console.debug(idt + '[' + key + '] => (Array) : ');
						rtn += idt + '[' + key + '] => (Array) : ' + _dump_array(o[key], indent + 1);
					}
					else
					{
						console.debug(idt + '[' + key + '] => (Object) : ');
						rtn += idt + '[' + key + '] => (Object) : ' + _dump_object(o[key], indent + 1);
					}
					break;
				default : 
					console.debug(idt + '[' + key +'] => Unknown data type');
					rtn += idt + '[' + key + "] => Unknown data type\n";
					break;
			}
		}
		
		rtn += "\n";
		return rtn;
	}
	
	var rtn = _dump_object(o);
	
	return rtn;
}

function dump_array_buffer(ab)
{
    var rtn = '';
    var len = ab.byteLength;
    var view = new Uint8Array(ab);
    for (var i = 0; i < len; i ++) {
        console.debug('[' + i + ']:' + view[i] + ' ');
        rtn += '[' + i + ']:' + view[i] + ' ';
    }
    return rtn;
}
