<?php
// $Id:$

/**
 * @file
 * Uc recurring subscription UI.
 */

/**
 * Create the subscription overiew page.
 */
function uc_recurring_subscription_overview() {
  $product_class = variable_get('uc_recurring_subscription_product_class', 'uc_recurring_subscription');
  $result = db_query("SELECT n.nid, n.title 
                      FROM {node} n
                        LEFT JOIN {uc_recurring_subscription} rs ON rs.nid=n.nid
                      WHERE n.type = '%s'
                      ORDER BY rs.weight", $product_class);

  $roles = user_roles(TRUE);
  
  $form['products']['#tree'] = TRUE;
  $form['products']['#theme'] = 'uc_recurring_subscription_products';
  while($node = db_fetch_object($result)) {
    $product = node_load($node->nid);

    // roles
    if (!empty($product->subscription)) {
      $product_roles = array_intersect_key($roles, $product->subscription->roles);
    }
    // payment options
    $products = _uc_recurring_subscription_product_feature($node->nid);
    $intervals = array();
    foreach ($products as $id => $feature) {
      $intervals[] = uc_price($feature->fee_amount, array()) . t(' charged every '). $feature->regular_interval; 
    }

    $form['products'][$node->nid]['title'] = array('#value' => l($product->title, 'node/'. $product->nid));
    $form['products'][$node->nid]['role'] = array('#value' => implode(', ', $product_roles));
    $form['products'][$node->nid]['interval'] = array('#value' => implode('<br/>', $intervals));
    $form['products'][$node->nid]['operations'] = array(
      '#value' => l('edit', 'admin/store/subscriptions/'. $node->nid .'/edit') .' | '. l('delete', 'node/'. $node->nid .'/delete', array('query' => 'admin/store/subscriptions'))
    );
    $form['products'][$node->nid]['weight'] = array(
      '#type' => 'weight',
      '#delta' => 10,
      '#default_value' => $product->subscription->weight,
    );
  }

  $form['submit'] = array('#type' => 'submit', '#value' => t('Save order'));

  return $form;
}

/**
 * Update the weights of the products.
 */
function uc_recurring_subscription_overview_submit($form, &$form_state) {
  foreach($form_state['values']['products'] as $nid => $product) {
    db_query("UPDATE {uc_recurring_subscription} SET weight = %d WHERE nid = %d", $product['weight'], $nid);
  }
}

/**
 * Form to add a new payment interval for the product.
 */
function uc_recurring_subscription_product_form($form_state, $product_id = NULL) {
  drupal_add_js(drupal_get_path('module', 'uc_recurring_subscription') .'/uc_recurring_subscription.js', 'module');
  drupal_add_css(drupal_get_path('module', 'uc_recurring_subscription') .'/uc_recurring_subscription.css');

  $form = array();
  if (isset($product_id)) {
    $form['product_id'] = array(
      '#type' => 'hidden',
      '#value' => $product_id,
    );
    $node = node_load($product_id);
  }

  $form['product'] = array(
    '#type' => 'fieldset',
    '#title' => 'Step One: Product Details',
  );
  $form['product']['title'] = array(
    '#type' => 'textfield',
    '#title' => 'Title',
    '#required' => TRUE,
    '#default_value' => $node->title,
  );
  $form['product']['body'] = array(
    '#type' => 'textarea',
    '#title' => 'Description',
    '#default_value' => $node->body,
  );

  $form['recurring'] = array(
    '#type' => 'fieldset',
    '#title' => 'Step Two: Define Subscription Details',
    '#description' => t('By adding multiple payment options, users will be given the option to choose from one of these payment options, this allows you to specify different prices and payment plans for this subscription product'),
  );
  $form['recurring']['add'] = array(
    '#type' => 'button',
    '#value' => 'Add a payment option',
    '#ahah' => array(
      'event' => 'click',
      'path' => 'subscriptions/ahah/add_interval',
      'wrapper' => 'recurring_intervals',
      'method' => 'replace',
      'effect' => 'fade',
      'progress' => array(
        'type' => 'bar',
      ),
    ),
    '#prefix' => '<div style="clear:both"></div>',
  );

  $form['recurring']['recurring_intervals'] = array(
    '#value' => t(' '),
    '#prefix' => '<div id="recurring_intervals">',
    '#suffix' => '</div>',
  );

  if (isset($form_state['recurring_count'])) {
    $recurring_count = $form_state['recurring_count'];
  }
  else {
    $products = _uc_recurring_subscription_product_feature($node->nid);
    $recurring_count = count($products);
  }
  $form['recurring']['recurring_intervals']['#theme'] = 'uc_recurring_subscription_item';
  for ($delta = 0; $delta < $recurring_count; $delta++) {
    $form['recurring']['recurring_intervals'][$delta] = _uc_recurring_subscription_add_interval_form($delta, $products[$delta]);
  }

  $form['role'] = array(
    '#type' => 'fieldset',
    '#title' => 'Step Three: Role Assignment',
    '#description' => t('Select the role(s) which are assigned to members who subscribe to this subscription product.'),
  );
  $roles = user_roles(TRUE);
  unset($roles[DRUPAL_AUTHENTICATED_RID]);
  if (empty($roles)) {
    $form['role']['message'] = array(
      '#value' => t('You need to first add a user role <a href="@role_link">here</a>.', array('@role_link' => url('admin/user/roles'))),
    );
  }
  else {
    $form['role']['subscribe'] = array(
      '#type' => 'select',
      '#title' => 'Role awarded at subscription purchase',
      '#options' => $roles,
      '#multiple' => TRUE,
      '#default_value' => $node->subscription->roles,
    );
  }

/*
  $form['email'] = array(
    '#type' => 'fieldset',
    '#title' => 'Step Four: Email/Notifications',
    '#description' => t('Setup emails to be sent on various events.'),
  );
  $form['email']['#theme'] = 'uc_recurring_subscription_email_options';
  $form['email']['notication_event'] = array(
    '#type' => 'select',
    '#title' => t('Add new email on event'),
    '#options' => array(),
  );
  $form['email']['add_notification'] = array('#type' => 'submit', '#value' => t('Add email'));
*/

  if (!empty($node->nid) && $node->type != variable_get('uc_recurring_subscription_product_class', 'uc_recurring_subscription')) {
    return confirm_form($form, '', uc_referer_uri(), t('This product is not current a subscription, saving this form will convert this product to a subscription product, which could result in data being lost if you have specified custom fields.'), t('Save product and convert to a subscription'), t('Cancel'));
  }
  else {
    $form['submit'] = array('#type' => 'submit', '#value' => t('Save'));
  }

  if ($product_id != -1) {
    $form['delete'] = array('#type' => 'submit', '#value' => t('Delete'));
  }

  return $form;
}

/**
 *
 */
function uc_recurring_subscription_ahah($context) {
  $func = '_uc_recurring_subscription_ahah_'. $context;
  $form_state['values'] = $_POST;
  if (function_exists($func)) {
    $func($form_state);
  }
  exit();
}

/**
 *
 */
function _uc_recurring_subscription_add_interval_form($delta, $product = NULL) {
  $form = array(
    '#tree' => TRUE,
  );

  $form['pfid'] = array(
    '#type' => 'hidden',
    '#value' => $product->pfid,
    '#parents' => array('recurring', $delta, 'pfid'),
  );

  $form['default'] = array(
    '#type' => 'radio',
    '#parents' => array('default_payment'),
    '#value' => $product->pfid,
  );
  $form['interval_value'] = array(
    '#type' => 'textfield',
    '#default_value' => $product->regular_interval_value,
    '#size' => 2,
    '#prefix' => '<div class="subscription-interval-value">',
    '#suffix' => '</div>',
    '#parents' => array('recurring', $delta, 'interval_value'),
  );
  $form['interval_unit'] = array(
    '#type' => 'select',
    '#options' => array(
      'days' => t('day(s)'),
      'weeks' => t('week(s)'),
      'months' => t('month(s)'),
      'years' => t('year(s)'),
    ),
    '#default_value' => $product->regular_interval_unit,
    '#prefix' => '<div class="subscripion-interval-period">',
    '#suffix' => '</div>',
    '#parents' => array('recurring', $delta, 'interval_unit'),
  );
  $form['amount'] = array(
    '#type' => 'textfield',
    '#default_value' => $product->fee_amount,
    '#size' => 10,
    '#prefix' => '<div class="subscription-amount">',
    '#suffix' => '</div>',
    '#parents' => array('recurring', $delta, 'amount'),
  );
  $form['number_intervals'] = array(
    '#type' => 'textfield',
    '#default_value' => $product->number_intervals < 0 ? '' : $product->number_intervals,
    '#attributes' => $product->number_intervals < 0 ? array('disabled' => 'disabled') : array(),
    '#size' => 10,
    '#prefix' => '<div class="subscription-num-intervals">',
    '#suffix' => '</div>',
    '#parents' => array('recurring', $delta, 'number_intervals'),
  );
  $attributes['class'] = 'unlimited-checkbox';
  if ($product->number_intervals < 0) {
    $attributes['checked'] = 'checked';
  }
  $form['unlimited'] = array(
    '#type' => 'checkbox',
    '#title' => t('Unlimited'),
    '#attributes' => $attributes,
    '#prefix' => '<div class="subscription-unlimited-intervals">',
    '#suffix' => '</div>',
    '#parents' => array('recurring', $delta, 'unlimited'),
  );
  $form['weight'] = array(
    '#type' => 'weight',
    '#delta' => 10,
    '#default_value' => $delta,
  );
  $form['operations'] = array(
    '#type' => 'button',
    '#value' => t('remove'),
    '#parents' => array('recurring', $delta, 'operations'),
    '#ahah' => array(
      'event' => 'click',
      'path' => 'subscriptions/ahah/remove_interval/'. $delta,
      'wrapper' => 'recurring_intervals',
      'method' => 'replace',
      'progress' => array(
        'type' => 'throbber',
      ),
    ),
  );
  return $form;
}

/**
 *
 */
function _uc_recurring_subscription_ahah_add_interval() {
  $delta = sizeof($_POST['recurring']) ? max(array_keys($_POST['recurring']))+1 : 0;
  $fields = _uc_recurring_subscription_add_interval_form($delta);
  
  $form_state = array('submitted' => FALSE);
  $form_build_id = $_POST['form_build_id'];
  // Add the new element to the stored form. Without adding the element to the
  // form, Drupal is not aware of this new elements existence and will not
  // process it. We retreive the cached form, add the element, and resave.
  $form = form_get_cache($form_build_id, $form_state);
  $form['recurring']['recurring_intervals'][$delta] = $fields;
  form_set_cache($form_build_id, $form, $form_state);
  $form += array(
    '#post' => $_POST,
    '#programmed' => FALSE,
  );
  // Rebuild the form.
  $form = form_builder($_POST['form_id'], $form, $form_state);

  // Render the new output.
  $new_form = $form['recurring']['recurring_intervals'];
  $output = drupal_render($new_form);

  drupal_json(array('data' => $output, 'status' => true));
}

/**
 *
 */
function _uc_recurring_subscription_ahah_remove_interval() {
  $form_state = array('submitted' => FALSE);
  $form_build_id = $_POST['form_build_id'];

  $delta = arg(3);
  $form = form_get_cache($form_build_id, $form_state);
  $pfid = $form['recurring']['recurring_intervals'][$delta]['pfid']['#value'];
  if (!empty($pfid)) {
    uc_recurring_fee_product_delete($pfid);
  }
  unset($form['recurring']['recurring_intervals'][$delta]);
  // Rebuild the form.
  $form = form_builder($_POST['form_id'], $form, $form_state);

  // Render the new output.
  $new_form = $form['recurring']['recurring_intervals'];
  $output = drupal_render($new_form);

  print drupal_to_js(array('data' => $output, 'status' => true));
}

/**
 *
 */
function uc_recurring_subscription_product_form_validate(&$form, &$form_state) {
  // Make the changes we want to the form state.
  if ($form_state['values']['recurring']) {
    $form_state['recurring_count'] = count($form_state['values']['recurring']);
  }

  if ($form_state['values']['op'] == 'submit') {
    if ($form_state['recurring_count'] <= 0) {
      form_set_error('recurring', t('You must specify at least one subscription/payment interval'));
    }
  }
}

/**
 *
 */
function uc_recurring_subscription_product_form_submit(&$form, &$form_state) {
  global $user;
  switch ($form_state['values']['op']) {
    case 'Delete':
      // @todo add confirmation page
      node_delete($form_state['values']['product_id']);
      break;
    default: // Save
      if (empty($form_state['values']['product_id'])) {
        $node = new stdClass();
        $node->type = variable_get('uc_recurring_subscription_product_class', 'uc_recurring_subscription');
        $node->uid = $user->uid;
        $node->status = 1;
        $new = TRUE;
        node_save($node);
      }
      else {
        $node = node_load($form_state['values']['product_id']);
        if ($node->type != variable_get('uc_recurring_subscription_product_class', 'uc_recurring_subscription')) {
          $node->type = variable_get('uc_recurring_subscription_product_class', 'uc_recurring_subscription');
        }
        $new = FALSE;
      }

      if (!empty($node)) {
        $node->title = $form_state['values']['title'];
        $node->body = $form_state['values']['body'];
        $node->list_price = $node->sell_price = $form_state['values']['recurring'][0]['amount'];

        // Attribute.
        $attribute = uc_attribute_load(variable_get('uc_recurring_subscription_attribute', ''));
        // Product attribute record.
        $prod_attr = db_fetch_object(db_query("SELECT * FROM {uc_product_attributes} WHERE nid = %d AND aid = %d", $node->nid, $attribute->aid));
        // we are going to rewrite the product adjustment records.
        db_query("DELETE FROM {uc_product_adjustments} WHERE nid = %d", $node->nid);

        $subscription = new stdClass();
        $subscription->nid = $node->nid;
        $subscription->roles = $form_state['values']['subscribe'];

        $key = empty($node->subscription) ? NULL : 'nid';
        drupal_write_record('uc_recurring_subscription', $subscription, $key);

        $features = array();
        // add/update the recurring features
        foreach ($form_state['values']['recurring'] as $index => $value) {
          $product->pfid = $value['pfid'];

          // @todo based on the subscription intervals we need to set up an attribute for this product
          $product->model = 'sub-'. $node->nid .'-'. $product->pfid;

          $product->fee_amount = $value['amount'];
          $product->initial_charge = $value['interval_value'] .' '. $value['interval_unit'];
          $product->regular_interval = $value['interval_value'] .' '. $value['interval_unit'];
          // If number intervals is negative, it means that it's unlimited intervals.
          $product->number_intervals = !empty($value['number_intervals']) ? $value['number_intervals'] : -1;
          $product->nid = $node->nid;

          // get option or create if it doesn't exist.
          $option_name = _uc_recurring_subscription_create_attribute_option($value['interval_value'], $value['interval_unit']);
          $option = _uc_recurring_subscription_attribute_option_by_name($attribute->aid, $option_name);
          if (empty($option)) {
            $option = new stdClass();
            $option->aid = $attribute->aid;
            $option->name = $option_name;
            uc_attribute_option_save($option);
          }
          
          // Set the default product options.
          if ($value['pfid'] == $form_state['values']['default_payment']) {
            $prod_attr->default_option = $option->oid;
            $node->model = $product->model;
          }
 
          // add the product option
          $prod_option = db_fetch_object(db_query("SELECT * FROM {uc_product_options} WHERE nid = %d AND oid = %d", $node->nid, $option->oid));
          $key = array('nid', 'oid');
          if (empty($prod_option)) {
            $prod_option = new stdClass();
            $prod_option->nid = $node->nid;
            $prod_option->oid = $option->oid;
            $key = NULL;
          }
          $prod_option->weight = $value['weight'];
          drupal_write_record('uc_product_options', $prod_option, $key);

          $prod_adj = new stdClass();
          $prod_adj->nid = $node->nid;
          $prod_adj->combination = serialize(array($attribute->aid => $option->oid));
          $prod_adj->model = $product->model;
          drupal_write_record('uc_product_adjustments', $prod_adj);

          uc_recurring_product_feature_save($product);
          $features[] = $product->pfid;
        }
 
        if (count($form_state['values']['recurring']) <= 1) {
          $prod_attr->required = 0;
        }
        else {
          $prod_attr->required = 1;
        }
        drupal_write_record('uc_product_attributes', $prod_attr, array('nid', 'aid'));

        node_save($node);
      }
      break;
  }
  $form_state['redirect'] = 'admin/store/subscriptions';
}

/**
 *
 */
function uc_recurring_subscription_subscriber_list($form_state, $product_id = -1) {
  $form = array();
  return $form;
}

/**
 *
 */
function uc_recurring_subscription_settings_form($form_state) {
  $form = array();

  $options = array();
  $result = db_query("SELECT * FROM {uc_product_classes}");
  while ($class = db_fetch_array($result)) {
    $options[$class['pcid']] = $class['name'];
  }
  $form['uc_recurring_subscription_product_class'] = array(
    '#type' => 'select',
    '#title' => 'Product Class',
    '#description' => 'Only products from this class will be managed as subscription.',
    '#options' => $options,
    '#default_value' => variable_get('uc_recurring_subscription_product_class', 'uc_recurring_subscription'),
  );

  $options = array();
  $attributes = uc_class_get_attributes(variable_get('uc_recurring_subscription_product_class', 'uc_recurring_subscription'));
  foreach($attributes as $id => $value) {
    $options[$id] = $value->name;
  }
  $form['uc_recurring_subscription_attribute'] = array(
    '#type' => 'select',
    '#title' => 'Payment Attribute',
    '#description' => 'The attribute used to provide the payment options to users.',
    '#options' => $options,
    '#default_value' => variable_get('uc_recurring_subscription_attribute', ''),
  );

  return system_settings_form($form);
}

/**
 *
 */
function theme_uc_recurring_subscription_item($form) {
  $header = array(t('Default'), t('Subscription Interval'), t('Amount'), t('Billing Intervals'), t('Order'), '');
  drupal_add_tabledrag('recurring-subscription', 'order', 'sibling', 'recurring-weight');

  foreach (element_children($form) as $key) {
    // Add class to group weight fields for drag and drop.
    $form[$key]['weight']['#attributes']['class'] = 'recurring-weight';

    $row = array();
    $row[] = drupal_render($form[$key]['default']);
    $row[] = drupal_render($form[$key]['interval_value']) .
             drupal_render($form[$key]['interval_unit']);
    $row[] = drupal_render($form[$key]['amount']);
    $row[] = drupal_render($form[$key]['number_intervals']) .
             drupal_render($form[$key]['unlimited']);
    $row[] = drupal_render($form[$key]['weight']);
    $row[] = drupal_render($form[$key]['operations']) . drupal_render($form[$key]['pfid']);
    $rows[] = array('data' => $row, 'class' => 'draggable tabledrag-leaf');
  }
  $output = theme('table', $header, $rows, array('id' => 'recurring-subscription'));
  $output .= drupal_render($form);
  return $output;
}

/**
 * Add table drag effect to product commission table.
 */
function theme_uc_recurring_subscription_products($form) {
  $header = array(t('Name'), t('Role'), t('Payment Options'), '');
  drupal_add_tabledrag('recurring-subscription-products', 'order', 'sibling', 'product-weight');

  foreach (element_children($form) as $key) {
    // Add class to group weight fields for drag and drop.
    $form[$key]['weight']['#attributes']['class'] = 'product-weight';

    $row = array();
    $row[] = drupal_render($form[$key]['title']);
    $row[] = drupal_render($form[$key]['role']);
    $row[] = drupal_render($form[$key]['interval']);
    $row[] = drupal_render($form[$key]['weight']);
    $row[] = drupal_render($form[$key]['operations']);
    $rows[] = array('data' => $row, 'class' => 'draggable');
  }

  if (empty($rows)) {
    $rows[] = array(
      array(
        'data' => l('Add new subscription', 'admin/store/subscriptions/create'),
        'colspan' => 5,
      ),
    );
  }

  $output = theme('table', $header, $rows, array('id' => 'recurring-subscription-products'));
  $output .= drupal_render($form);
  return $output;
}
