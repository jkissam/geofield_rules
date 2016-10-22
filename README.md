# Geofield Rules

Drupal module that provides Rules for interacting with [geofields](https://www.drupal.org/project/geofield). Currently being developed by [Jonathan Kissam](http://jonathankissam.com/).

_10-22-2016_
Current version (7.x-1.x-dev) only provides a single rule, which takes a latitude and a longitude and searches a given geofield (which must be a polygon). It returns the entity_id of the first entity which contains the point in the geofield's polygon (it also returns the entity, but this hasn't been tested yet in practice). It was developed for [Let's Grow Kids](http://letsgrowkids.org), to use with state legislative districts - which are available as shapefiles from [census.gov](https://www.census.gov/geo/maps-data/data/cbf/cbf_sld.html).

Find this module useful, or want to see more features? Please consider supporting further development by [hiring me or making a donation](http://jonathankissam.com/support).