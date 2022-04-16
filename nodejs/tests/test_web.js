const chai = require('chai')
const chaiHttp = require('chai-http')
chai.use(chaiHttp)
const assert = chai.assert
const app = require('../app')
const { describe, it } = require('mocha')

describe('# Test Web App', function () {
  it('# Test GET /', async function () {
    const res = await chai.request(app).get('/')
    assert.equal(res.body.user, 'none')
  })
})
