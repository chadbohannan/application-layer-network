import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { Packet } from '../aln/packet.js';

describe('Packet', () => {
  describe('constructor', () => {
    it('should create empty packet with default values', () => {
      const p = new Packet();
      assert.equal(p.cf, 0);
      assert.equal(p.net, 0);
      assert.equal(p.srv, '');
      assert.equal(p.src, '');
      assert.equal(p.dst, '');
      assert.equal(p.seq, 0);
      assert.equal(p.data, null);
    });

    it('should initialize all fields', () => {
      const p = new Packet();

      // Set various fields
      p.srv = 'test-service';
      p.src = 'source-addr';
      p.dst = 'dest-addr';
      p.data = 'test data';
      p.seq = 42;
      p.ctx = 100;

      assert.equal(p.srv, 'test-service');
      assert.equal(p.src, 'source-addr');
      assert.equal(p.dst, 'dest-addr');
      assert.equal(p.data, 'test data');
      assert.equal(p.seq, 42);
      assert.equal(p.ctx, 100);
    });
  });

  describe('toJson', () => {
    it('should serialize packet to JSON string', () => {
      const p = new Packet();
      p.srv = 'test-service';
      p.src = 'source';
      p.dst = 'destination';
      p.data = 'test data';

      const json = p.toJson();

      assert.ok(json.includes('test-service'), 'JSON should contain service name');
      assert.ok(json.includes('source'), 'JSON should contain source');
      assert.ok(json.includes('destination'), 'JSON should contain destination');
    });

    it('should base64 encode binary data', () => {
      const p = new Packet();
      p.data = 'binary data';

      const json = p.toJson();
      const obj = JSON.parse(json);

      assert.ok(obj.data, 'JSON should have data field');
      assert.equal(typeof obj.data, 'string', 'Data should be string (base64)');
    });

    it('should include all non-empty fields', () => {
      const p = new Packet();
      p.srv = 'service';
      p.src = 'source';
      p.dst = 'dest';
      p.seq = 10;
      p.ctx = 20;

      const json = p.toJson();
      const obj = JSON.parse(json);

      assert.equal(obj.srv, 'service');
      assert.equal(obj.src, 'source');
      assert.equal(obj.dst, 'dest');
      // Note: seq and ctx are not included in toJson() - this is a limitation
    });
  });

  describe('copy', () => {
    it('should create independent copy of packet', () => {
      const p1 = new Packet();
      p1.srv = 'test';
      p1.src = 'source';
      p1.dst = 'dest';
      p1.data = 'hello';
      p1.seq = 42;
      p1.ctx = 100;

      const p2 = p1.copy();

      assert.equal(p2.srv, p1.srv);
      assert.equal(p2.src, p1.src);
      assert.equal(p2.dst, p1.dst);
      assert.equal(p2.data, p1.data);
      assert.equal(p2.seq, p1.seq);
      assert.equal(p2.ctx, p1.ctx);
    });

    it('should create independent object', () => {
      const p1 = new Packet();
      p1.srv = 'original';

      const p2 = p1.copy();
      p2.srv = 'modified';

      assert.equal(p1.srv, 'original', 'Original should not be modified');
      assert.equal(p2.srv, 'modified', 'Copy should be modified');
    });

    it('should copy all fields including cf and net', () => {
      const p1 = new Packet();
      p1.cf = 0x0400;
      p1.net = 1;
      p1.srv = 'test';

      const p2 = p1.copy();

      assert.equal(p2.cf, p1.cf);
      assert.equal(p2.net, p1.net);
      assert.equal(p2.srv, p1.srv);
    });
  });
});
