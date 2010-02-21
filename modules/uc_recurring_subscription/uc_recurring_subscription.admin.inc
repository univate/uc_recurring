<?php
// $Id:$

/**
 * @file
 * Uc recurring subscription UI.
 */

/**
 * Create the subscription overiew page.
 */
function uc_recurring_subscripion_overview() {
  $header = array(
    array('data' => t('Name'), 'field' => 'n.title'),
    array('data' => t('Role')),
    array('data' => t('Payment Period(s)')),
    '');

  $product_class = variable_get('uc_recurring_subscription_product_class', 'uc_recurring_subscription');

  $sql = db_rewrite_sql("SELECT n.nid, n.title FROM {node} n WHERE n.type = '%s'");
  $sql_count = db_rewrite_sql("SELECT COUNT(n.nid) FROM {node} n WHERE n.type = '%s'", $product_class);

  $result = pager_query($sql, 20, 0, $sql_count, $product_class);

  $roles = user_roles(TRUE);

  while($node = db_fetch_object($result)) {
    $product = node_load($node->nid);

    // roles
    if (!empty($product->subscription)) {
      $product_roles = array_intersect_key($roles, $product->subscription->roles);
    }

    $row = array();
    $row['title'] = l($product->title, 'node/'. $product->nid);
    $row['role'] = implode(', ', $product_roles);
    $row['interval'] = '';
    $row['operations'] = l('edit', 'admin/store/subscriptions/'. $product->nid .'/edit') .' | '. l('delete', 'node/'. $product->nid .'/delete');
    $rows[] = array('data' => $row);
  }

  if (empty($rows)) {
    $rows[] = array(
      array(
        'data' => l('Add new subscription', 'admin/store/subscriptions/create'),
        'colspan' => 4,
      ),
    );
  }

  return theme('table', $header, $rows, array('id' => 'uc-recurring-subscriptions-ui-overview'));
}

/**
 * Form to add a new payment interval for the product.
 */
function uc_recurring_subscripion_product_form($form_state, $product_id = NULL) {
  drupal_add_js(drupal_get_path('module', 'uc_recurring_subscripion') .'/uc_recurring_subscripion.js', 'module');
  drupal_add_css(drupal_get_path('module', 'uc_recurring_subscripion') .'/uc_recurring_subscripion.css');

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
      'method' => 'append',
      'effect' => 'fade',
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
    $products = _uc_recurring_subscripion_product_feature($node->nid);
    $recurring_count = count($products);
  }
  for ($delta = 0; $delta < $recurring_count; $delta++) {
    $form['recurring']['recurring_intervals'][$delta] = _uc_recurring_subscripion_add_interval_form($delta, $products[$delta]);
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
function uc_recurring_subscripion_ahah($context) {
  $func = '_uc_recurring_subscripion_ahah_'. $context;
  $form_state['values'] = $_POST;
  if (function_exists($func)) {
    $func($form_state);
  }
  exit();
}

/**
 *
 */
function _uc_recurring_subscripion_add_interval_form($delta, $product = NULL) {
  $form = array(
    '#tree' => TRUE,
  );

  $form['pfid'] = array(
    '#type' => 'hidden',
    '#value' => $product->pfid,
    '#parents' => array('recurring', $delta, 'pfid'),
    '#prefix' => '<div style="clear: both;">',
  );

  $form['interval_value'] = array(
    '#type' => 'textfield',
    '#title' => t('Subscription Interval'),
    '#default_value' => $product->regular_interval_value,
    '#required' => TRUE,
    '#size' => 10,
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
    '#title' => t('Renewal Cost'),
    '#default_value' => $product->fee_amount,
    '#size' => 10,
    '#prefix' => '<div class="subscription-amount">',
    '#suffix' => '</div>',
    '#parents' => array('recurring', $delta, 'amount'),
  );
  $form['number_intervals'] = array(
    '#type' => 'textfield',
    '#title' => t('Billing Cycles'),
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
  $form['operations'] = array(
    '#type' => 'button',
    '#value' => t('remove'),
    '#attributes' => array('style' => 'margin: 2.6em 0 0 2em;'),
    '#suffix' => '</div>',
    '#parents' => array('recurring', $delta, 'operations'),
    '#ahah' => array(
      'event' => 'click',
      'path' => 'subscriptions/ahah/remove_interval',
      'wrapper' => 'recurring_intervals',
      'method' => 'append',
    ),
  );
  return $form;
}

/**
 *
 */
function _uc_recurring_subscripion_ahah_add_interval() {
  $delta = sizeof($_POST['recurring']) ? max(array_keys($_POST['recurring']))+1 : 0;
  $fields = _uc_recurring_subscripion_add_interval_form($delta);

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
  $new_form = $form['recurring']['recurring_intervals'][$delta];
  $output = drupal_render($new_form);

  print drupal_to_js(array('data' => $output, 'status' => true));
}

function _uc_recurring_subscripion_ahah_remove_interval() {
  $form_state = array('submitted' => FALSE);
  $form_build_id = $_POST['form_build_id'];

  $form = form_get_cache($form_build_id, $form_state);
  // Rebuild the form.
  $form = form_builder($_POST['form_id'], $form, $form_state);
  print '';
}

/**
 *
 */
function uc_recurring_subscripion_product_form_validate(&$form, &$form_state) {
  // Make the changes we want to the form state.
  if ($form_state['values']['recurring']) {
    $form_state['recurring_count'] = count($form_state['values']['recurring']);
  }

  if ($form_state['recurring_count'] <= 0) {
    form_set_error('recurring', t('You must specify at least one subscription/payment interval'));
  }
}

/**
 *
 */
function uc_recurring_subscripion_product_form_submit(&$form, &$form_state) {
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
        node_save($node);

        $subscription = new stdClass();
        $subscription->nid = $node->nid;
        $subscription->roles = $form_state['values']['subscribe'];

        if (empty($node->subscription)) {
          drupal_write_record('uc_recurring_subscription', $subscription);
        }
        else {
          drupal_write_record('uc_recurring_subscription', $subscription, array('nid'));
        }

        $features = array();
        // add/update the recurring features
        foreach ($form_state['values']['recurring'] as $index => $value) {
          $product->pfid = $value['pfid'];

          // @todo based on the subscription intervals we need to set up an attribute for this product
          // $product->model = $form_state['values']['model'];

          $product->fee_amount = $value['amount'];
          $product->initial_charge = $value['interval_value'] .' '. $value['interval_unit'];
          $product->regular_interval = $value['interval_value'] .' '. $value['interval_unit'];
          // If number intervals is negative, it means that it's unlimited intervals.
          $product->number_intervals = !empty($value['number_intervals']) ? $value['number_intervals'] : -1;
          $product->nid = $node->nid;

          uc_recurring_product_feature_save($product);
          $features[] = $product->pfid;

          // add/update the role feature for this payment option
          // uc_roles does not expose any good API's so we need to hack out a solution using drupal_execute
        }

        // remove and features deleted
        $result = db_query("SELECT * FROM {uc_product_features} WHERE nid = %d", $product->nid);
        while ($feature = db_fetch_object($result)) {
          if (!in_array($feature->pfid, $features)) {

          }
        }

      }
      break;
  }
  $form_state['redirect'] = 'admin/store/subscriptions';
}

/**
 *
 */
function uc_recurring_subscripion_subscriber_list($form_state, $product_id = -1) {
  $form = array();
  return $form;
}

/**
 *
 */
function uc_recurring_subscripion_settings_form($form_state) {
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
    '#default_value' => variable_get('uc_recurring_subscription_attributes', ''),
  );

  return system_settings_form($form);
}
