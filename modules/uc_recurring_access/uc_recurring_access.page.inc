<?php
// $Id$

/**
 * @file
 *
 */

/**
 * Form to add/remove recurring access rules for each node.
 */
function uc_recurring_access_node_form($form_state, $node) {
  drupal_set_title(t('Recurring access rules for @title', array('@title' => $node->title)));
  if (!uc_recurring_access_content_type($node)) {
    $form['recurring_acess'] = array(
      '#value' => t('Recurring access not applicable to this content type.'),
    );
    return $form;
  }
  drupal_add_js('misc/autocomplete.js');

  $form['message'] = array(
    '#value' => t('<p>This content can be restricted to only allow user accounts which match one of the following recurring product access rules.</p>'),
  );

  $form['recurring_access']['#theme'] = 'uc_recurring_access_items_items';
  $form['recurring_access']['#tree'] = TRUE;
  // add new rule form
  $rule_types = _uc_recurring_access_rules();
  $form['recurring_access']['new']['access_type'] = array(
    '#type' => 'select',
    '#options' => array('' => 'Add recurring access rule for ...') + $rule_types,
    '#ahah' => array(
      'event' => 'change',
      'path' => 'uc_recurring_access/ahah/access_type',
      'wrapper' => 'recurring-access-type',
      'method' => 'replace',
      'effect' => 'fade',
      'progress' => array(
      ),
    ),
  );
  $form['recurring_access']['new']['match'] = array(
    '#value' => '<div id="recurring-access-type"></div>'
  );
  $form['recurring_access']['new']['time_required'] = array(
    '#type' => 'textfield',
    '#description' => t('Enter a PHP time, e.g. 1 week'),
    '#size' => 20,
  );

  $count = 0;
  $rules = uc_recurring_access_node_load($node->nid);
  foreach ($rules as $rule) {
    $form['recurring_access'][$count]['access_type'] = array(
      '#value' => $rule_types[$rule->access_type],
    );
    $form['recurring_access'][$count]['time_required'] = array(
      '#value' => theme('uc_recurring_time', $rule->time_required),
    );
    switch ($rule->access_type) {
      case 'node':
        $product = node_load($rule->id);
        $value = l($product->title, 'nid/'. $rule->id);
        break;
      case 'role':
        static $roles;
        if (!isset($roles)) {
          $roles = user_roles(TRUE);
        }
        $value = $roles[$rule->id];
    }
    $form['recurring_access'][$count]['match'] = array(
      '#value' => $value,
    );
    $form['recurring_access'][$count]['id'] = array(
      '#type' => 'hidden',
      '#value' => $rule->id,
    );
    $form['recurring_access'][$count]['access'] = array(
      '#type' => 'hidden',
      '#value' => $rule->access_type,
    );
    $form['recurring_access'][$count]['delete'] = array(
      '#type' => 'checkbox',
    );
    $count++;
  }
  $form['nid'] = array(
    '#type' => 'value',
    '#value' => $node->nid,
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  return $form;
}

/**
 * Save new access rules and delete old ones.
 */
function uc_recurring_access_node_form_submit($form, &$form_state) {
  $nid = $form_state['values']['nid'];

  // save the new record
  if ($form_state['values']['recurring_access']['new']['access_type'] != '') {
    $access = new stdClass();
    $access->nid = $nid;
    $access->time_required = strtotime('+'. $form_state['values']['recurring_access']['new']['time_required'], 0);
    $access->access_type = $form_state['values']['recurring_access']['new']['access_type'];
    $access->id = $form_state['values']['match'];
    uc_recurring_access_node_save($access);
  }

  // delete the ones that have been checked.
  foreach ($form_state['values']['recurring_access'] as $rule_id => $access) {
    if ($access['delete'] == 1) {
      // delete this record
      $access_type = $form_state['values']['recurring_access'][$rule_id]['access'];
      $id = $form_state['values']['recurring_access'][$rule_id]['id'];
      uc_recurring_access_node_delete($nid, $access_type, $id);
    }
  }
}

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
