import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import {
  NET_ROUTE,
  NET_SERVICE,
  NET_QUERY,
  makeNetQueryPacket,
  makeNetworkRouteSharePacket,
  parseNetworkRouteSharePacket,
  makeNetworkServiceSharePacket,
  parseNetworkServiceSharePacket
} from '../aln/utils.js';

describe('Utils', () => {
  describe('makeNetQueryPacket', () => {
    it('should create NET_QUERY packet', () => {
      const p = makeNetQueryPacket();
      assert.equal(p.net, NET_QUERY);
      assert.equal(p.net, 3);
    });
  });

  describe('route packets', () => {
    it('should create route advertisement packet', () => {
      const p = makeNetworkRouteSharePacket('source', 'destination', 5);

      assert.equal(p.net, NET_ROUTE);
      assert.equal(p.net, 1);
      assert.equal(p.src, 'source');
      assert.ok(p.data, 'Should have data payload');
      assert.ok(p.sz > 0, 'Should have data size');
    });

    it('should parse route advertisement packet', () => {
      const p = makeNetworkRouteSharePacket('router1', 'router2', 7);

      const [dest, nextHop, cost, err] = parseNetworkRouteSharePacket(p);

      assert.equal(err, null);
      assert.equal(dest, 'router2');
      assert.equal(nextHop, 'router1');
      assert.equal(cost, 7);
    });

    it('should roundtrip route packet', () => {
      const original = makeNetworkRouteSharePacket('source-addr', 'dest-addr', 42);
      const [dest, nextHop, cost, err] = parseNetworkRouteSharePacket(original);

      assert.equal(err, null);
      assert.equal(dest, 'dest-addr');
      assert.equal(nextHop, 'source-addr');
      assert.equal(cost, 42);
    });

    it('should handle route withdrawal (cost 0)', () => {
      const p = makeNetworkRouteSharePacket('router1', 'router2', 0);
      const [dest, nextHop, cost, err] = parseNetworkRouteSharePacket(p);

      assert.equal(err, null);
      assert.equal(cost, 0);
    });

    it('should reject non-route packet', () => {
      const p = makeNetQueryPacket();
      const [dest, nextHop, cost, err] = parseNetworkRouteSharePacket(p);

      assert.notEqual(err, null);
      assert.ok(err.includes('NET_ROUTE'));
    });
  });

  describe('service packets', () => {
    it('should create service advertisement packet', () => {
      const p = makeNetworkServiceSharePacket('host-addr', 'my-service', 10);

      assert.equal(p.net, NET_SERVICE);
      assert.equal(p.net, 2);
      assert.ok(p.data, 'Should have data payload');
      assert.ok(p.sz > 0, 'Should have data size');
    });

    it('should parse service advertisement packet', () => {
      const p = makeNetworkServiceSharePacket('server1', 'api', 15);

      const [addr, serviceID, capacity, err] = parseNetworkServiceSharePacket(p);

      assert.equal(err, null);
      assert.equal(addr, 'server1');
      assert.equal(serviceID, 'api');
      assert.equal(capacity, 15);
    });

    it('should roundtrip service packet', () => {
      const original = makeNetworkServiceSharePacket('my-host', 'my-svc', 99);
      const [addr, serviceID, capacity, err] = parseNetworkServiceSharePacket(original);

      assert.equal(err, null);
      assert.equal(addr, 'my-host');
      assert.equal(serviceID, 'my-svc');
      assert.equal(capacity, 99);
    });

    it('should handle service removal (capacity 0)', () => {
      const p = makeNetworkServiceSharePacket('server1', 'api', 0);
      const [addr, serviceID, capacity, err] = parseNetworkServiceSharePacket(p);

      assert.equal(err, null);
      assert.equal(capacity, 0);
    });

    it('should reject non-service packet', () => {
      const p = makeNetQueryPacket();
      const [addr, serviceID, capacity, err] = parseNetworkServiceSharePacket(p);

      assert.notEqual(err, null);
      assert.ok(err.includes('NET_ROUTE')); // Error message says NET_ROUTE (bug in error message)
    });

    it('should handle long service names', () => {
      const longName = 'a'.repeat(200);
      const p = makeNetworkServiceSharePacket('host', longName, 5);
      const [addr, serviceID, capacity, err] = parseNetworkServiceSharePacket(p);

      assert.equal(err, null);
      assert.equal(serviceID, longName);
    });

    it('should handle long host addresses', () => {
      const longAddr = 'b'.repeat(200);
      const p = makeNetworkServiceSharePacket(longAddr, 'service', 5);
      const [addr, serviceID, capacity, err] = parseNetworkServiceSharePacket(p);

      assert.equal(err, null);
      assert.equal(addr, longAddr);
    });
  });
});
