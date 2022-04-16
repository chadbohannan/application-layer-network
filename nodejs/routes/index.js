const logger = require('../logging/logger')
const router = require('express').Router()

router.get('/', (req, res) => {
  logger.info('getting /')
  res.status(200).json({ user: 'none' })
})

module.exports = router
