<?php
// $Id$

/**
 * @file
 *
 */

/**
 * This is just a helper function for handling ahah callbacks.
 */
function uc_recurring_access_ahah($context) {
  $func = '_uc_recurring_access_ahah_'. $context;
  $form_state['values'] = $_POST;
  if (function_exists($func)) {
    $func($form_state);
  }
  exit();
}

/**
 *
 */
function _uc_recurring_access_ahah_access_type($form_state) {
  switch ($form_state['values']['recurring_access']['new']['access_type']) {
    case '':
      $form['match'] = array('#value' => '');
      break;
    case 'node':
      $form['match'] = array(
        '#type' => 'textfield',
        '#size' => 20,
        '#autocomplete_path' => 'uc_recurring_access/product/autocomplete',
      );
      break;
    case 'role':
      $roles = user_roles(TRUE);
      $form['match'] = array(
        '#type' => 'select',
        '#options' => $roles,
      );
      break;
    default:
       $form['match'] = array(
        '#type' => 'textfield',
        '#size' => 20,
       );
  }

  $output = _uc_recurring_access_ahah_render($form, 'match');
  drupal_json(array('data' => $output, 'status' => true));
}

/*
 * This function is largely based on the poll module, its been simplified for reuse.
 *   $fields is the specific form elements you want to attach via ahah,
 *   $name is the form fields array key... e.g. the name for $form['title'] is "title"
 */
function _uc_recurring_access_ahah_render($fields, $name) {
  $form_state = array('submitted' => FALSE);
  $form_build_id = $_POST['form_build_id'];
  // Add the new element to the stored form. Without adding the element to the
  // form, Drupal is not aware of this new elements existence and will not
  // process it. We retreive the cached form, add the element, and resave.
  $form = form_get_cache($form_build_id, $form_state);
  $form[$name] = $fields;
  form_set_cache($form_build_id, $form, $form_state);
  $form += array(
    '#post' => $_POST,
    '#programmed' => FALSE,
  );
  // Rebuild the form.
  $form = form_builder($_POST['form_id'], $form, $form_state);

  // Render the new output.
  $new_form = $form[$name];
  return drupal_render($new_form);
}

function uc_recurring_access_product_autocomplete($string) {
  if (is_numeric($string)) {
    // search for product node id.
    $sql = "SELECT n.nid, n.title
            FROM {node} n
              LEFT JOIN { uc_recurring_products} p ON p.nid = n.nid
            WHERE n.nid LIKE '%d%%'
            GROUP BY n.nid";
  }
  else {
    $sql = "SELECT n.nid, n.title
            FROM {node} n
              LEFT JOIN {uc_recurring_products} p ON p.nid = n.nid
            WHERE n.title LIKE '%s%%'
            GROUP BY n.nid";
  }
  $result = db_query($sql, $string);
  $items = array();
  while($obj = db_fetch_object($result)) {
    $items[$obj->nid] = check_plain($obj->title) . ' [ID: ' . $obj->nid . ']';
  }
  drupal_json($items);
  exit();
}
