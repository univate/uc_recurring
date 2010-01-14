<?php
// $Id: uc_recurring.admin.inc,v 1.1.4.10 2010/01/08 05:59:55 univate Exp $

/**
 * @file
 * Recurring payments administration menu items.
 *
 */

// Displays a table for the administration of recurring fees.
function uc_recurring_admin() {
  $item = menu_get_item();

  $fees = array();

  $header = array(
    array('data' => t('ID'), 'field' => 'rfid'),
    array('data' => t('Order'), 'field' => 'order_id'),
    array('data' => t('Amount')),
    array('data' => t('Next'), 'field' => 'next_charge', 'sort' => 'asc'),
    array('data' => t('Interval')),
    array('data' => t('Left')),
    array('data' => t('Total')),
    array('data' => t('Operations')),
  );

  if (empty($item['map']['5'])) {
    // Get all recurring fees in the system.
    $order = tablesort_sql($header);
    $element = 0;
    if (empty($_GET['page'])) {
      $index = db_result(db_query("SELECT COUNT(*) FROM {uc_recurring_users} WHERE next_charge < NOW()"));
      $_GET['page'] = $index / variable_get('uc_order_number_displayed', 30);
    }
    $fees= uc_recurring_get_all_fees(TRUE, $order);
  }
  elseif ($item['map']['5'] == 'order') {
    // Get all recurring fees in an order.
    $order = uc_order_load($item['map']['6']);
    $fees = uc_recurring_get_fees($order);
  }
  elseif ($fee = uc_recurring_fee_user_load($item['map'][6])) {
    // Get single recurring fee.
    $fees[] = $fee;
  }

  $output = drupal_get_form('uc_recurring_admin_filter_form', $item);
  if ($fees) {
    $context = array(
      'revision' => 'themed-original',
      'location' => 'recurring-admin',
    );

    foreach ($fees as $fee) {
      $ops = uc_recurring_get_fee_ops('fee_admin', $fee);
      $rows[] = array(
        l($fee->rfid, 'admin/store/orders/recurring/view/fee/'. $fee->rfid),
        l($fee->order_id, 'admin/store/orders/'. $fee->order_id),
        $fee->fee_amount != '0.00' ? uc_price($fee->fee_amount, $context) : t('Same as product price'),
        format_date($fee->next_charge, 'small'),
        array('data' => check_plain($fee->regular_interval), 'nowrap' => 'nowrap'),
        $fee->remaining_intervals < 0 ? t('Until cancelled') : $fee->remaining_intervals,
        $fee->remaining_intervals < 0 ? $fee->charged_intervals : $fee->remaining_intervals + $fee->charged_intervals,
        array('data' => implode(' ', $ops), 'nowrap' => 'nowrap'),
      );
    }

    $output .= theme('table', $header, $rows);
    $output .= theme('pager', NULL, variable_get('uc_order_number_displayed', 30), 0);

  }
  $output .= l(t('Back to the full list'), 'admin/store/orders/recurring');

  return $output;
}

/**
 * Filter by a specific order ID.
 *
 * @param $item
 *   A menu item.
 */
function uc_recurring_admin_filter_form($form_state, $item) {
  $form['type'] = array(
    '#type' => 'select',
    '#options' => array(
      'order' => t('Order ID'),
      'fee' => t('Fee ID'),
    ),
    '#default_value' => $item['map'][5] == 'fee' ? 'fee' : 'order',
    '#prefix' => '<div style="float: left; margin-right: 1em;">',
    '#suffix' => '</div>',
  );
  $form['id'] = array(
    '#type' => 'textfield',
    '#default_value' => $item['map'][6],
    '#size' => 10,
    '#prefix' => '<div style="float: left; margin-right: 1em;">',
    '#suffix' => '</div>',
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Filter'),
    '#attributes' => array('style' => 'margin: .85em 0em;'),
  );

  return $form;
}

function uc_recurring_admin_filter_form_submit($form, &$form_state) {
  if (intval($form_state['values']['id']) > 0) {
    $form_state['redirect'] = 'admin/store/orders/recurring/view/'. $form_state['values']['type'] .'/'. $form_state['values']['id'];
  }
}


/**
 * Confirm a recurring fee charge.
 *
 * @param $form_state
 * @param $uid
 * @return unknown_type
 */
function uc_recurring_admin_charge_form($form_state, $rfid, $fee_handler) {
  $fee = uc_recurring_fee_user_load($rfid);
  if ($fee->fee_handler == $fee_handler) {
    $context = array(
      'revision' => 'formatted-original',
      'location' => 'recurring-charge-form',
    );
    $form['rfid'] = array(
      '#type' => 'hidden',
      '#value' => $fee->rfid
    );
  
  
    $form['message'] = array(
      '#value' => '<div>'. t('Are you sure you want to charge the customer @amount at this time?', array('@amount' => uc_price($fee->fee_amount, $context))) .'</div>',
    );
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Process Renewal'),
      '#suffix' => l(t('Cancel'), uc_referer_uri()),
    );
    return $form;
  }
  else {
    drupal_set_message(t('Invalid fee handler was given, try again from the operations for this <a href="@url">recurring fee</a>, if problem persists contact the site administrator.', array('@url' => url(uc_referer_uri()))), 'error');
  }
}

function uc_recurring_admin_charge_form_submit($form, &$form_state) {
  $rfid = $form_state['values']['rfid'];
  $fee = uc_recurring_fee_user_load($rfid);

  // Attempt to renew the charge.
  if (uc_recurring_renew($fee)) {
    drupal_set_message(t('Recurring fee @fee charged successfully.', array('@fee' => $rfid)));
  }
  else {
    drupal_set_message(t('Attempt to charge recurring fee @fee failed.', array('@fee' => $rfid)), 'error');
  }

  $form_state['redirect'] = 'admin/store/orders/recurring/view/fee/'. $rfid;
}

/**
 * Let an admin edit a recurring fee.
 */
function uc_recurring_admin_edit_form($form_state, $rfid, $fee_handler) {
  $fee = uc_recurring_fee_user_load($rfid);
  if ($fee->fee_handler == $fee_handler) {
    drupal_add_css(drupal_get_path('module', 'uc_recurring') .'/uc_recurring.css');
    drupal_add_js(drupal_get_path('module', 'uc_recurring') .'/uc_recurring.js', 'module');
  
    list($fee->regular_interval_value, $fee->regular_interval_unit) = explode(' ', $fee->regular_interval);
  
    $form['fee_amount'] = array(
      '#type' => 'textfield',
      '#title' => t('Recurring fee amount'),
      '#description' => t('Charge this amount each billing period.'),
      '#default_value' => $fee->fee_amount,
      '#size' => 16,
      '#field_prefix' => variable_get('uc_sign_after_amount', FALSE) ? '' : variable_get('uc_currency_sign', '$'),
      '#field_suffix' => variable_get('uc_sign_after_amount', FALSE) ? variable_get('uc_currency_sign', '$') : '',
    );
  
    $form['num_interval'] = array(
      '#type' => 'fieldset',
      '#title' => t('Number of billing periods'),
      '#collapsible' => FALSE,
      '#description' => t('Specify how many more recurring fees will be charged.'),
    );
  
    if ($fee->remaining_intervals < 0) {
      $attributes['checked'] = 'checked';
    }
    $form['num_interval']['unlimited_intervals'] = array(
      '#type' => 'checkbox',
      '#title' => t('Unlimited rebillings.'),
      '#attributes' => $attributes,
    );
    $form['num_interval']['number_intervals'] = array(
      '#type' => 'textfield',
      '#title' => t('Remaining billing periods'),
      '#size' => 16,
      '#default_value' => $fee->remaining_intervals < 0 ? '' : $fee->remaining_intervals,
      '#attributes' => $fee->remaining_intervals < 0 ? array('disabled' => 'disabled') : array(),
    );
  
    $form['regular'] = array(
      '#type' => 'fieldset',
      '#title' => t('Regular interval'),
      '#collapsible' => FALSE,
      '#description' => t('Modify the length of the billing period for this fee. Changing this value will reset the timer for the next charge. You can also charge the fee manually to collect payment ahead of time and reset the interval.'),
      '#attributes' => array('class' => 'interval-fieldset'),
    );
    $form['regular']['regular_interval_value'] = array(
      '#type' => 'select',
      '#options' => drupal_map_assoc(uc_range(1, 52)),
      '#default_value' => $fee->regular_interval_value,
    );
    $form['regular']['regular_interval_unit'] = array(
      '#type' => 'select',
      '#options' => array(
        'days' => t('day(s)'),
        'weeks' => t('week(s)'),
        'months' => t('month(s)'),
        'years' => t('year(s)'),
      ),
      '#default_value' => $fee->regular_interval_unit,
    );
  
    $form['reset_next_charge'] = array(
      '#type' => 'checkbox',
      '#title' => t('Reset the next charge timer upon form submission using the specified interval.'),
      '#description' => t('Checking this .'),
    );
  
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Submit'),
      '#suffix' => l(t('Cancel'), uc_referer_uri()),
    );
  
    return $form;
  }
  else {
    drupal_set_message(t('Invalid fee handler was given, try again from the operations for this <a href="@url">recurring fee</a>, if problem persists contact the site administrator.', array('@url' => url(uc_referer_uri()))), 'error');
  }
}

function uc_recurring_admin_edit_form_validate($form, &$form_state) {
  if (empty($form_state['values']['unlimited_intervals']) && intval($form_state['values']['number_intervals']) < 0) {
    form_set_error('number_intervals', t('Only positive whole number values are accepted for the number of billing periods.'));
  }
}

function uc_recurring_admin_edit_form_submit($form, &$form_state) {
  global $user;
  $rfid = arg(4); 
  $fee = uc_recurring_fee_user_load($rfid);

  $interval = $form_state['values']['regular_interval_value'] .' '. $form_state['values']['regular_interval_unit'];

  $form_state['values']['number_intervals'] = empty($form_state['values']['unlimited_intervals']) ? $form_state['values']['number_intervals'] : UC_RECURRING_UNLIMITED_INTERVALS;

  db_query("UPDATE {uc_recurring_users} SET fee_amount = %f, regular_interval = '%s', "
          ."remaining_intervals = %d WHERE rfid = %d", $form_state['values']['fee_amount'],
           $interval, $form_state['values']['number_intervals'], $rfid);

  if ($form_state['values']['reset_next_charge']) {
    $next_charge = strtotime('+'. $interval);
    db_query("UPDATE {uc_recurring_users} SET next_charge = %d WHERE rfid = %d", $next_charge, arg(4));
  }

  drupal_set_message(t('The changes to the fee have been saved.'));

  // Fee was already cancelled and now has been resumed, add a comment to the order.
  if ($fee->remaining_intervals == 0 && $form_state['values']['number_intervals'] != 0) {
    uc_order_comment_save($fee->order_id, $user->uid, t('<a href="@user-link">@user</a> resumed the recurring fee <a href="@fee-link">@fee</a>.', array('@user-link' => url('user/'. $user->uid), '@user' => $user->name, '@fee-link' => url('admin/store/orders/recurring/view/fee/'. $rfid), '@fee'=> $rfid)));
  }
  // If the number of intervals set to zero then also call cancel.
  if ($fee->remaining_intervals != 0 && $form_state['values']['number_intervals'] == 0) {
    uc_recurring_fee_cancel($rfid, $fee);
  }

  $form_state['redirect'] = 'admin/store/orders/recurring/view/fee/'. arg(4);
}

