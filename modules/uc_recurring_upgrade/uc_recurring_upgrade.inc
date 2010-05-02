<?php
// $Id$

/**
 * @file
 * Provides a way to upgrade a recurring product.
 *
 * Initial module development sponsored by Marketing Results
 * http://www.marketing-results.com.au/
 */

/**
 * Autocomplete function for finding recurring products.
 */
function uc_recurring_upgrade_autocomplete($string) {
  $items = array();
  if (is_numeric($string)) {
    $sql = "SELECT n.nid, n.title
            FROM {node} n
              INNER JOIN {uc_recurring_products} rp ON rp.nid = n.nid
            WHERE n.nid LIKE '%d%%'";
  } else {
    $sql = "SELECT n.nid, n.title
            FROM {node} n
              INNER JOIN {uc_recurring_products} rp ON rp.nid = n.nid
            WHERE n.title LIKE '%s%%'";
  }

  $result = db_query($sql, $string);

  while($obj = db_fetch_object($result)) {
    $items[$obj->nid] = check_plain($obj->title) . ' [ID: ' . $obj->nid . ']';
  }
  drupal_json($items);
  exit();
}

/**
 * This is just a helper function for handling ahah callbacks.
 */
function uc_recurring_upgrade_ahah($context) {
  $func = '_uc_recurring_upgrade_ahah_'. $context;
  $form_state['values'] = $_POST;
  if (function_exists($func)) {
    $func($form_state);
  }
  exit();
}
  
/**
 * Load the product module options.
 */
function _uc_recurring_upgrade_ahah_product_model() {
  $form_state = array('submitted' => FALSE);
  $form_build_id = $_POST['form_build_id'];
  // Add the new element to the stored form. Without adding the element to the
  // form, Drupal is not aware of this new elements existence and will not
  // process it. We retreive the cached form, add the element, and resave.
  $form = form_get_cache($form_build_id, $form_state);
  $node = node_load($_POST['product_from_id']);
  if (in_array($node->type, uc_product_types())) {
    $options = uc_product_get_models($node);
    $form['product_from']['product_from_model'] = array(
      '#type' => 'select',
      '#title' => t('SKU'),
      '#description' => t('Select the applicable product model/SKU for the product @product.', array('@product' => $node->title)),
      '#options' => $options,
    );
  }
  else {
    $form['product_from']['product_from_model'] = array(
      '#value' => t('No valid product'),
    );
  }
  form_set_cache($form_build_id, $form, $form_state);
  $form += array(
    '#post' => $_POST,
    '#programmed' => FALSE,
  );
  // Rebuild the form.
  $form = form_builder($_POST['form_id'], $form, $form_state);

  // Render the new output.
  $new_form = $form['product_from']['product_from_model'];
  $output = drupal_render($new_form);

  drupal_json(array('data' => $output, 'status' => true));
}


