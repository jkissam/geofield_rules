<?php

/**
 * implements hook_rules_action_info
 * creates custom action for rules
 */
function geofield_rules_rules_action_info() {

	return array(
		'geofield_rules_entities_contain_point' => array(
			'label' => t('Select entity that contains a point within its geofield'),
			'group' => t('Geofield'),
			'parameter' => array(
				'geofield' => array(
					'label' => t('Geofield'),
					'type' => 'text',
					'options list' => '_geofield_rules_geofields',
				),
				'latitude' => array(
					'label' => t('Latitude'),
					'type' => 'text',
				),
				'longitude' => array(
					'label' => t('Longitude'),
					'type' => 'text',
				),
			),
			'provides' => array(
				'entity_id' => array(
					'type' => 'integer',
					'label' => t('Returned entity id'),	
				),
				'entity' => array(
					'type' => 'entity',
					'label' => t('Returned entity'),
				),
			),
		),
		
	);

}

/**
 * callback function for getting entities that contain a given point
 */
function geofield_rules_entities_contain_point($geofield,$latitude,$longitude) {

	list($entity_type,$bundle,$field_name) = explode(':',$geofield);
	$geom_field = $field_name.'_geom';
	
	$entity_id = 0;
	$debug_message = '';
	
	// first, do a database query	
	$query_string = 'SELECT entity_id, asText(GeomFromWKB('.$geom_field.')) AS geom FROM {field_data_field_district_polygon} f WHERE  (entity_type = :entity_type) AND (bundle = :bundle) AND (CONTAINS(GeomFromWKB('.$geom_field.'), POINT(:longitude, :latitude)))';
	$debug_message .= "query_string: $query_string\n";
	$result = db_query($query_string, array(
		':entity_type' => $entity_type,
		':bundle' => $bundle,
		':longitude' => $longitude,
		':latitude' => $latitude,	
	));
	
	// MySQL geometry queries use MBR, which potentially return false "contains"
	// so we have to test results using geoPHP
	while ( ($record = $result->fetchObject() ) && !$entity_id) {
		$debug_message .= "Record:\n".print_r($record,1)."\n";
		$polygon = geoPHP::load($record->geom,'wkt');
		$point = geoPHP::load("POINT($longitude $latitude)",'wkt');
		if ($polygon->geometryType() != 'Polygon') { continue; }
		
		// if GEOS is installed, use it
		if (geoPHP::geosInstalled() && $polygon->contains($point)) {
			$entity_id = $record->entity_id;
			
		// otherwise, use our custom function
		} elseif (_geofield_rules_point_in_polygon($polygon, $point)) {
			$entity_id = $record->entity_id;
		}
	}

	if ($entity_id) {
		return array(
			'entity_id' => $entity_id,
			'entity' => entity_load($entity_type, array($entity_id)),
		);
	} else {
		return array(
			'entity_id' => 0,
			'entity' => null,
		);	
	}
}

/**
 * helper function to get list of available geofields, with the bundles they are attached to
 */
function _geofield_rules_geofields() {
	$query = db_select('field_config_instance','fci');
	$query->leftJoin('field_config','fc','fci.field_id = fc.id');
	$query->fields('fci',array('entity_type','bundle'));
	$query->fields('fc',array('field_name'));
	$query->condition('fc.type','geofield','=');
	$query->orderBy('fc.field_name');
	$result = $query->execute();
	$return = array();
	while($record = $result->fetchAssoc()) {
		$field_info = field_info_instance($record['entity_type'], $record['field_name'],$record['bundle']);
		$field_label = isset($field_info['label']) ? $field_info['label'] : $record['field_name'];
		$entity_info = entity_get_info($record['entity_type']);
		$entity_label = isset($entity_info['label']) ? $entity_info['label'] : $record['entity_type'];
		$bundle_label = isset($entity_info['bundles'][$record['bundle']]['label']) ? $entity_info['bundles'][$record['bundle']]['label'] : $record['bundle'];
		$return[$record['entity_type'].':'.$record['bundle'].':'.$record['field_name']] = $field_label . ' (' . $entity_label . ' ' . $bundle_label . ')';
	}
	return $return;
}

/**
 * helper function to test whether a point is within a given polygon, in case GEOS is not installed
 */
function _geofield_rules_point_in_polygon($polygon, $point, $pointOnBoundary = true, $pointOnVertex = true) {
  $linestring = $polygon->getComponents();
  $vertices = $linestring[0]->getComponents();

  // check if point is on a vertex
  foreach ($vertices as $v) {
    if ( ($v->x() == $point->x()) && ( $v->y() == $point->y() ) ) {
      return $pointOnVertex ? TRUE : FALSE;
    }
  }

    // Check if the point is inside the polygon or on the boundary
    $intersections = 0; 
    $vertices_count = count($vertices);
    for ($i=1; $i < $vertices_count; $i++) {
      $vertex1 = $vertices[$i-1]; 
      $vertex2 = $vertices[$i];
      if ($vertex1->y() == $vertex2->y() 
      && $vertex1->y() == $point->y() 
      && $point->x() > min($vertex1->x(), $vertex2->x()) 
      && $point->x() < max($vertex1->x(), $vertex2->x())) {
        // Check if point is on an horizontal polygon boundary
        return $pointOnBoundary ? TRUE : FALSE;
      }
      if ($point->y() > min($vertex1->y(), $vertex2->y())
      && $point->y() <= max($vertex1->y(), $vertex2->y())
      && $point->x() <= max($vertex1->x(), $vertex2->x())
      && $vertex1->y() != $vertex2->y()) {
        $xinters = 
          ($point->y() - $vertex1->y()) * ($vertex2->x() - $vertex1->x())
          / ($vertex2->y() - $vertex1->y()) 
          + $vertex1->x();
        if ($xinters == $point->x()) {
          // Check if point is on the polygon boundary (other than horizontal)
          return $pointOnBoundary ? TRUE : FALSE;
        }
        if ($vertex1->x() == $vertex2->x() || $point->x() <= $xinters) {
          $intersections++;
        }
      } 
    }

    if ($intersections % 2 != 0) {
      return TRUE;
    }
    else {
      return FALSE;
    }

}