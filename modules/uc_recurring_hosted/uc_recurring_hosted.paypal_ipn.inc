<?php
// $id$

/**
 * @file
 * Handle paypal IPN callbacks for recurring payments
 */

/**
 * Handle IPN callbacks for recurring payments
 */
function uc_recurring_hosted_paypal_ipn($order_id) {
  watchdog('uc_recurring_hosted', 'Receiving IPN at URL for order @order_id. @debug',
    array('@order_id' => $order_id, '@debug' => variable_get('uc_paypal_wps_debug_ipn', FALSE) ? '<pre>'. print_r($_POST, TRUE) .'</pre>' : ''));

  if (!isset($_POST['invoice'])) {
    watchdog('uc_recurring_hosted', 'IPN attempted with invalid order ID.', array(), WATCHDOG_ERROR);
    return;
  }

  if (($len = strpos($_POST['invoice'], '-')) > 0) {
    $order_id = intval(substr($_POST['invoice'], 0, $len));
  }
  else {
    $order_id = intval($_POST['invoice']);
  }

  $order = uc_order_load($order_id);

  if ($order == FALSE) {
    watchdog('uc_recurring_hosted', 'IPN attempted for non-existent order.', array(), WATCHDOG_ERROR);
    return;
  }

  // Assign posted variables to local variables
  $payment_status = check_plain($_POST['payment_status']);
  $payment_amount = check_plain($_POST['mc_gross']);
  $payment_currency = check_plain($_POST['mc_currency']);
  $payment_fee = check_plain($_POST['mc_fee']);
  $receiver_email = check_plain($_POST['receiver_email']);
  $txn_id = check_plain($_POST['txn_id']);
  $txn_type = check_plain($_POST['txn_type']);
  $payer_email = check_plain($_POST['payer_email']);

  $req = '';

  if ($order == FALSE) {
    watchdog('uc_recurring_hosted', 'IPN attempted for non-existent order.', array(), WATCHDOG_ERROR);
    return;
  }

  foreach ($_POST as $key => $value) {
    $value = urlencode(stripslashes($value));
    $req .= $key .'='. $value .'&';
  }

  $req .= 'cmd=_notify-validate';

  if (variable_get('uc_paypal_wpp_server', '') == 'https://api-3t.paypal.com/nvp') {
    $host = 'https://www.paypal.com/cgi-bin/webscr';
  }
  else {
    $host = variable_get('uc_paypal_wps_server', 'https://www.sandbox.paypal.com/cgi-bin/webscr');
  }

  $response = drupal_http_request($host, array(), 'POST', $req);

  // TODO: Change this to property_exists when we have a PHP requirement >= 5.1.
  if (array_key_exists('error', $response)) {
    watchdog('uc_recurring_hosted', 'IPN failed with HTTP error @error, code @code.', array('@error' => $response->error, '@code' => $response->code), WATCHDOG_ERROR);
    return;
  }

  if (strcmp($response->data, 'VERIFIED') == 0) {
    watchdog('uc_recurring_hosted', 'IPN transaction verified.');

    // subscription
    $subscr_id = check_plain($_POST['subscr_id']);

    switch ($txn_type) {
      case 'subscr_signup':
        // first we need to setup the recurring fee, since paypal_wps overrides the submit order function we need to call uc_recurring_order submit function here
        uc_recurring_order('submit', $order, NULL);
        $txn_id = $subscr_id; // subscriptions do not have a txn_id, so we will record the subscr_id instead
        break;
      case 'subscr_payment':
        if ($order->order_status == 'completed') {
          // Fetch fee from database.
          $fees = uc_recurring_get_fees($order);
          if ($fee = $fees[0]) {
            $order_id = uc_recurring_renew($fee);
            $order = uc_order_load($order_id);
          }
        }
        break;
      case 'subscr_failed':
        // Calculate when the next retry will be and then extend.

        //
        return;
      case 'subscr_eot':
      case 'subscr_cancel':
        $fees = uc_recurring_get_fees($order);
        if ($fee = $fees[0]) {
          uc_recurring_fee_cancel($fee->rfid, $fee);
        }
        $txn_id = $subscr_id;
        break;
    }

    $duplicate = db_result(db_query("SELECT COUNT(*) FROM {uc_payment_paypal_ipn} WHERE txn_id = '%s' AND txn_type = '%s' AND status <> 'Pending'", $txn_id, $txn_type));
    if ($duplicate > 0) {
      if ($order->payment_method != 'credit') {
        watchdog('uc_recurring_hosted', 'IPN transaction ID has been processed before.', array(), WATCHDOG_NOTICE);
      }
      return;
    }

    db_query("INSERT INTO {uc_payment_paypal_ipn} (order_id, txn_id, txn_type, mc_gross, status, receiver_email, payer_email, received) VALUES (%d, '%s', '%s', '%s', '%s', '%s', '%s', %d)",
      $order_id, $txn_id, $txn_type, $payment_amount, $payment_status, $receiver_email, $payer_email, time());

    $context = array(
      'revision' => 'formatted-original',
      'type' => 'amount',
    );
    $options = array(
      'sign' => FALSE,
    );

    switch ($payment_status) {
      case 'Canceled_Reversal':
        uc_order_comment_save($order_id, 0, t('PayPal has cancelled the reversal and returned !amount !currency to your account.', array('!amount' => uc_price($payment_amount, $context, $options), '!currency' => $payment_currency)), 'admin');
        break;

      case 'Completed':
        $comment = t('PayPal transaction ID: @txn_id', array('@txn_id' => $txn_id));
        uc_payment_enter($order_id, 'paypal_wps', $payment_amount, $order->uid, NULL, $comment);
        uc_cart_complete_sale($order);
        uc_order_comment_save($order_id, 0, t('Payment of @amount @currency submitted through PayPal.', array('@amount' => uc_price($payment_amount, $context, $options), '@currency' => $payment_currency)), 'order', 'payment_received');
        uc_order_comment_save($order_id, 0, t('PayPal IPN reported a payment of @amount @currency.', array('@amount' => uc_price($payment_amount, $context, $options), '@currency' => $payment_currency)));
        break;

      case 'Denied':
        uc_order_comment_save($order_id, 0, t("You have denied the customer's payment."), 'admin');
        break;

      case 'Expired':
        uc_order_comment_save($order_id, 0, t('The authorization has failed and cannot be captured.'), 'admin');
        break;

      case 'Failed':
        uc_order_comment_save($order_id, 0, t("The customer's attempted payment from a bank account failed."), 'admin');
        break;

      case 'Pending':
        uc_order_update_status($order_id, 'paypal_pending');
        uc_order_comment_save($order_id, 0, t('Payment is pending at PayPal: @reason', array('@reason' => _uc_paypal_pending_message(check_plain($_POST['pending_reason'])))), 'admin');
        break;

      // You, the merchant, refunded the payment.
      case 'Refunded':
        $comment = t('PayPal transaction ID: @txn_id', array('@txn_id' => $txn_id));
        uc_payment_enter($order_id, 'paypal_wps', $payment_amount, $order->uid, NULL, $comment);
        break;

      case 'Reversed':
        watchdog('uc_recurring_hosted', 'PayPal has reversed a payment!', array(), WATCHDOG_ERROR);
        uc_order_comment_save($order_id, 0, t('Payment has been reversed by PayPal: @reason', array('@reason' => _uc_paypal_reversal_message(check_plain($_POST['reason_code'])))), 'admin');
        break;

      case 'Processed':
        uc_order_comment_save($order_id, 0, t('A payment has been accepted.'), 'admin');
        break;

      case 'Voided':
        uc_order_comment_save($order_id, 0, t('The authorization has been voided.'), 'admin');
        break;
    }
  }
  elseif (strcmp($response->data, 'INVALID') == 0) {
    watchdog('uc_recurring_hosted', 'IPN transaction failed verification.', array(), WATCHDOG_ERROR);
    uc_order_comment_save($order_id, 0, t('An IPN transaction failed verification for this order.'), 'admin');
  }
}

/**
 * Mock page for testing the Paypal verifry call.
 */
function _uc_recurring_hosted_paypal_mock_web_page() {
  echo 'VERIFIED';
  exit();
}


