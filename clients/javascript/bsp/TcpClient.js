/*
 * TcpClient.js
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
 * TCP client based on HTML5 WebSocket
 * New Firefox && Chrome support only now.
 * 
 * @package bsp
 * @author Dr.NP <np@bsgroup.org>
 * @update 10/11/2012
 * @changelog
 *         [10/11/2012] - Creation
 */

/**
 * Main Client class
 */

var client;

function TcpClient(host, port) {
    // Constants
    this.DATA_TYPE_STREAM       = 0x0;
    this.DATA_TYPE_PACKET       = 0x1;
    
    this.PACKET_TYPE_REP        = 0x0;
    this.PACKET_TYPE_RAW        = 0x1;
    this.PACKET_TYPE_OBJ        = 0x2;
    this.PACKET_TYPE_CMD        = 0x3;
    this.PACKET_TYPE_HEARTBEAT  = 0x7;
    
    this.SERIALIZE_TYPE_NATIVE  = 0x0;
    this.SERIALIZE_TYPE_JSON    = 0x1;
    this.SERIALIZE_TYPE_MSGPACK = 0x2;
    this.SERIALIZE_TYPE_AMF     = 0x3;
    
    this.COMPRESS_TYPE_NONE     = 0x0;
    this.COMPRESS_TYPE_DEFLATE  = 0x1;
    this.COMPRESS_TYPE_LZ4      = 0x2;
    this.COMPRESS_TYPE_SNAPPY   = 0x3;
    
    // Properties
    this.host                   = host;
    this.port                   = port;
    //this.socket                 = null;
    //this.on_connect             = null;
    //this.on_close               = null;
    //this.on_data                = null;
    //this.on_error               = null;
    //this.on_heartbeat           = null;
    this.heartbeat              = 0;
    this.heartbeat_failure      = 10;
    this.heartbeat_check        = 10;
    this.heartbeat_req          = null;
    this.timer                  = null;
    this.recv_buffer            = null;
    
    // Data type
    this.data_type = (arguments[2] == 'packet') ? this.DATA_TYPE_PACKET : this.DATA_TYPE_STREAM;
    
    // Heartbeat
    this.heartbeat = arguments[3] || 0;

    // Heartbeat failure check
    this.heartbeat_failure = arguments[4] || 10;
    this.heartbeat_check = this.heartbeat_failure;
    client = this;
    
    return;
}

TcpClient.prototype._u82str = function(ab) {
    var binary_string = '', i;
    for (i = 0; i < ab.length; i ++) {
        binary_string += String.fromCharCode(ab[i]);
    }
    
    return binary_string;
};

TcpClient.prototype._hdr = function(packet_type) {
    var hdr = new Uint8Array(1);
    hdr[0] = (packet_type & 7) << 5 | this.SERIALIZE_TYPE_JSON << 2 | this.COMPRESS_TYPE_NONE;
    
    return hdr;
};

TcpClient.prototype._len = function(len) {
    var ret = '';
    if (0 === len >> 7) {
        // 1 Byte : 7Bits
        ret = String.fromCharCode(len & 0x7F);
    }
    else if (0 === len >> 14) {
        // 2 Bytes : 14Bits
        ret = String.fromCharCode(((len >> 7) & 0x7F) | 0x80) + 
            String.fromCharCode(len & 0x7F);
    }
    else if (0 === len >> 21) {
        // 3 Bytes : 21Bits
        ret = String.fromCharCode(((len >> 14) & 0x7F) | 0x80) + 
            String.fromCharCode(((len >> 7) & 0x7F) | 0x80) + 
            String.fromCharCode(len & 0x7F);
    }
    else if (0 === len >> 28) {
        // 4 Bytes : 28Bits
        ret = String.fromCharCode(((len >> 21) & 0x7F) | 0x80) + 
            String.fromCharCode(((len >> 14) & 0x7F) | 0x80) + 
            String.fromCharCode(((len >> 7) & 0x7F) | 0x80) + 
            String.fromCharCode(len & 0x7F);
    }
    else if (0 === len >> 35) {
        // 5 Bytes : 35Bits
        ret = String.fromCharCode(((len >> 28) & 0x7F) | 0x80) + 
            String.fromCharCode(((len >> 21) & 0x7F) | 0x80) + 
            String.fromCharCode(((len >> 14) & 0x7F) | 0x80) + 
            String.fromCharCode(((len >> 7) & 0x7F) | 0x80) + 
            String.fromCharCode(len & 0x7F);
    }
    else if (0 === len >> 28) {
        // 6 Bytes : 42Bits
        ret = String.fromCharCode(((len >> 35) & 0x7F) | 0x80) + 
            String.fromCharCode(((len >> 28) & 0x7F) | 0x80) + 
            String.fromCharCode(((len >> 21) & 0x7F) | 0x80) + 
            String.fromCharCode(((len >> 14) & 0x7F) | 0x80) + 
            String.fromCharCode(((len >> 7) & 0x7F) | 0x80) + 
            String.fromCharCode(len & 0x7F);
    }
    else if (0 === len >> 28) {
        // 7 Bytes : 49Bits
        ret = String.fromCharCode(((len >> 42) & 0x7F) | 0x80) + 
            String.fromCharCode(((len >> 35) & 0x7F) | 0x80) + 
            String.fromCharCode(((len >> 28) & 0x7F) | 0x80) + 
            String.fromCharCode(((len >> 21) & 0x7F) | 0x80) + 
            String.fromCharCode(((len >> 14) & 0x7F) | 0x80) + 
            String.fromCharCode(((len >> 7) & 0x7F) | 0x80) + 
            String.fromCharCode(len & 0x7F);
    }
    else if (0 === len >> 28) {
        // 8 Bytes : 56Bits
        ret = String.fromCharCode(((len >> 49) & 0x7F) | 0x80) + 
            String.fromCharCode(((len >> 42) & 0x7F) | 0x80) + 
            String.fromCharCode(((len >> 35) & 0x7F) | 0x80) + 
            String.fromCharCode(((len >> 28) & 0x7F) | 0x80) + 
            String.fromCharCode(((len >> 21) & 0x7F) | 0x80) + 
            String.fromCharCode(((len >> 14) & 0x7F) | 0x80) + 
            String.fromCharCode(((len >> 7) & 0x7F) | 0x80) + 
            String.fromCharCode(len & 0x7F);
    }
    else {
        // 9 Bytes : 64Bits
        ret = String.fromCharCode(((len >> 56) & 0x7F) | 0x80) + 
            String.fromCharCode(((len >> 49) & 0x7F) | 0x80) + 
            String.fromCharCode(((len >> 42) & 0x7F) | 0x80) + 
            String.fromCharCode(((len >> 35) & 0x7F) | 0x80) + 
            String.fromCharCode(((len >> 28) & 0x7F) | 0x80) + 
            String.fromCharCode(((len >> 21) & 0x7F) | 0x80) + 
            String.fromCharCode(((len >> 14) & 0x7F) | 0x80) + 
            String.fromCharCode(((len >> 7) & 0x7F) | 0x80) + 
            String.fromCharCode(len & 0x7F);
    }
    
    return ret;
};

TcpClient.prototype._parse_len = function(dv, curr) {
    var ret = new Object();
    var code = 0;
    ret.data = 0;
    ret.bytes = 0;
    
    // 1 Byte
    code = dv.getUint8(curr);
    ret.data = code & 0x7F;
    ret.bytes = 1;
    if (0 == (code & 0x80)) return ret;
    // 2 Bytes
    code = dv.getUint8(curr + 1);
    ret.data = (ret.data << 7) | (code & 0x7F);
    ret.bytes = 2;
    if (0 == (code & 0x80)) return ret;
    // 3 Bytes
    code = dv.getUint8(curr + 2);
    ret.data = (ret.data << 7) | (code & 0x7F);
    ret.bytes = 3;
    if (0 == (code & 0x80)) return ret;
    // 4 Bytes
    code = dv.getUint8(curr + 3);
    ret.data = (ret.data << 7) | (code & 0x7F);
    ret.bytes = 4;
    if (0 == (code & 0x80)) return ret;
    // 5 Bytes
    code = dv.getUint8(curr + 4);
    ret.data = (ret.data << 7) | (code & 0x7F);
    ret.bytes = 5;
    if (0 == (code & 0x80)) return ret;
    // 6 Bytes
    code = dv.getUint8(curr + 5);
    ret.data = (ret.data << 7) | (code & 0x7F);
    ret.bytes = 6;
    if (0 == (code & 0x80)) return ret;
    // 7 Bytes
    code = dv.getUint8(curr + 6);
    ret.data = (ret.data << 7) | (code & 0x7F);
    ret.bytes = 7;
    if (0 == (code & 0x80)) return ret;
    // 8 Bytes
    code = dv.getUint8(curr + 7);
    ret.data = (ret.data << 7) | (code & 0x7F);
    ret.bytes = 8;
    if (0 == (code & 0x80)) return ret;
    // 9 Bytes
    code = dv.getUint8(curr + 8);
    ret.data = (ret.data << 8) | (code & 0xFF);
    return ret;
};

var on_connect = function() {
    if (client.heartbeat > 0) {
        client.timer = window.setInterval(function() {
            if (client.socket.readyState == client.socket.OPEN) {
                client.socket.send(client.heartbeat_req);
            }
            client.heartbeat_check --;
            // Check failure
            if (client.heartbeat_check <= 0) {
                // Socket error ?
                var ev = new Event('BSP.Failure');
                document.dispatchEvent(ev);
                client.disconnect();
            }
        }, client.heartbeat, null);
    }
    while (this.readyState != this.OPEN) {}
    this.send(new Blob([client._hdr(client.PACKET_TYPE_REP)]));
    
    var ev = new Event('BSP.Connect');
    document.dispatchEvent(ev);
    
    return;
};

var on_close = function() {
    if (client.timer) {
        window.clearInterval(this.timer);
    }
    
    var ev = new Event('BSP.Close');
    document.dispatchEvent(ev);
    
    return;
};

var on_error = function(e) {
    if (client.timer) {
        window.clearInterval(this.timer);
    }
    
    console.log(e.data);
    var ev = new Event('BSP.Error', {'error' : e.data});
    document.dispatchEvent(ev);
    
    return;
};

var on_data = function(d) {
    if (!d.data instanceof Blob) {
        return;
    }
    
    client.recv_buffer = client.recv_buffer ? new Blob([client.recv_buffer, d.data]) : d.data;
    client.heartbeat_check = client.heartbeat_failure;
    var reader = new FileReader();
    if (client.DATA_TYPE_PACKET == client.data_type) {
        // Packet
        reader.addEventListener('loadend', _on_data_packet);
    }
    else {
        // Stream
        reader.addEventListener('loadend', _on_data_stream);
    }
    
    reader.readAsArrayBuffer(client.recv_buffer);
    
    return;
};

var _on_data_packet = function(e) {
    var buf = e.target.result;
    var curr = 0;
    var remaining = buf.byteLength;
    var dv = new DataView(buf, curr);
    while (remaining > 0) {
        // Packet header
        var hdr = dv.getUint8(curr);
        var p_type = (hdr >> 5) & 7;
        var s_type = (hdr >> 2) & 7;
        var c_type = (hdr) & 3;
        var ev;
        remaining --;
        curr ++;
        
        if (client.PACKET_TYPE_RAW == p_type || client.PACKET_TYPE_OBJ == p_type || client.PACKET_TYPE_CMD == p_type) {
            // Data
            var plen = client._parse_len(dv, curr);
            remaining -= plen.bytes;
            curr += plen.bytes;
            if (remaining < plen.data) break;
            var ab;
            var cmd;
            switch (p_type) {
                case client.PACKET_TYPE_RAW : 
                    ev = new CustomEvent('BSP.Data', {'detail' : {'type' : 'raw', 'raw' : buf.slice(curr, curr + plen.data)}});
                    document.dispatchEvent(ev);
                    break;
                case client.PACKET_TYPE_OBJ : 
                    ab = new Uint8Array(buf.slice(curr, curr + plen.data));
                    ev = new CustomEvent('BSP.Data', {'detail' : {'type' : 'obj', 'obj' : JSON.parse(client._u82str(ab))}});
                    document.dispatchEvent(ev);
                    break;
                case client.PACKET_TYPE_CMD : 
                    cmd = dv.getUint32(curr, false);
                    ab = new Uint8Array(buf.slice(curr + 4, curr + plen.data));
                    ev = new CustomEvent('BSP.Data', {'detail' : {'type' : 'cmd', 'cmd' : cmd, 'params' : JSON.parse(client._u82str(ab))}});
                    document.dispatchEvent(ev);
                    break;
                default : 
                    break;
            }
            remaining -= plen.data;
            curr += plen.data;
        }
        else if (client.PACKET_TYPE_HEARTBEAT == p_type) {
            // Heartbeat
            ev = new Event('BSP.Heartbeat');
            document.dispatchEvent(ev);
        }
        else if (this.PACKET_TYPE_REP == p_type) {
            // Rep, just ignore
        }
        else {
            // Unknown
        }
    }
    
    // Slice buffer
    if (0 == remaining) client.recv_buffer = null;
    else client.recv_buffer = client.recv_buffer.slice(curr);
};

var _on_data_stream = function(e) {
    var ev = new Event('BSP.Data', {'type' : 'stream', 'stream' : e.target.result});
    document.dispatchEvent(ev);
    client.recv_buffer = null;
};

TcpClient.prototype.connect = function() {
    var addr = 'ws://' + this.host + ':' + this.port + '/';
    if ('MozWebSocket' in window) {
        // Mozilla
        this.socket = new MozWebSocket(addr);
    }
    else if ('WebSocket' in window) {
        // Webkit
        this.socket = new WebSocket(addr);
    }
    else {
        // WebSocket not supported
        alert('You browser does not support WebSocket');
        console.log("BSP : You browser does not support WebSocket");
        return;
    }
    
    if (!this.socket) {
        console.log('BSP : Create websocket connection error');
    }
    
    // Events and type
    this.socket.binaryType = "blob";
    this.heartbeat_req = new Blob([this._hdr(this.PACKET_TYPE_HEARTBEAT)]);
    this.socket.onopen = on_connect;
    this.socket.onclose = on_close;
    this.socket.onmessage = on_data;
    this.socket.onerror = on_error;
    
    return;
};

TcpClient.prototype.disconnect = function() {
    if (this.timer) {
        clearInterval(this.timer);
    }
    if (this.socket) {
        this.socket.close();
    }
    //var ev = new Event('BSP.Close');
    //document.dispatchEvent(ev);
    
    return;
};

TcpClient.prototype._send_stream = function(data) {
    var b = new Blob([data]);
    this.socket.send(b);
    
    return b.length;
};

TcpClient.prototype._send_raw = function(data) {
    var hdr = this._hdr(this.PACKET_TYPE_RAW);
    var cnt = new Blob([data]);
    var len = this._len(cnt.size);
    var b = new Blob([hdr, len, cnt]);
    this.socket.send(b);
    
    return b.length;
};

TcpClient.prototype._send_obj = function(obj) {
    var hdr = this._hdr(this.PACKET_TYPE_OBJ);
    var cnt = new Blob([JSON.stringify(obj)]);
    var len = this._len(cnt.size);
    var b = new Blob([hdr, len, cnt]);
    this.socket.send(b);
    
    return b.length;
};

TcpClient.prototype._send_cmd = function(cmd, params) {
    var hdr = this._hdr(this.PACKET_TYPE_CMD);
    var cta = new ArrayBuffer(4);
    var dv = new DataView(cta);
    dv.setInt32(0, cmd, false);
    var cnt = new Blob([JSON.stringify(params)]);
    var len = this._len(cnt.size + 4);
    var b = new Blob([hdr, len, cta, cnt]);
    this.socket.send(b);
    
    return b.length;
};

TcpClient.prototype.send = function() {
    if (!this.socket) {
        return -1;
    }
    
    while (this.socket.readyState != this.socket.OPEN) {}
    var ret = -1;
    if (this.DATA_TYPE_STREAM == this.client_type) {
        // stream data
        var data = arguments[0] ? arguments[0] : '';
        if (typeof(data) == 'string') ret = this._send_stream(data); 
    }
    else {
        // JSON data
        var v1 = arguments[0] ? arguments[0] : null;
        if (typeof(v1) == 'number') {
            // Command
            var v2 = arguments[1] ? arguments[1] : new Object();
            if (typeof(v2) == 'object') ret = this._send_cmd(v1, v2);
        }
        else if (typeof(v1) == 'object') {
            // Object
            ret = this._send_obj(v1);
        }
        else if (typeof(v1) == 'string') {
            // Raw
            ret = this._send_raw(v1);
        }
    }
    
    return ret;
};
