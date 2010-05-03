<?php
// $Id: uc_recurring.uc_credit.inc,v 1.1.2.4 2009/10/15 12:59:41 univate Exp $

/**
 * @file
 * Uc recurring implementation for the credit card module.
 */

/**
 * Implementation of hook_recurring_info().
 */
function uc_recurring_uc_credit_recurring_info() {
  $gateway = uc_credit_default_gateway();
  $items['credit'] = array(
    'name' => t('Credit card'),
    'payment method' => 'credit',
    'module' => 'uc_recurring',
    'fee handler' => $gateway,
    'process callback' => 'uc_recurring_uc_credit',
    'renew callback' => 'uc_recurring_uc_credit',
    'cancel callback' => 'uc_recurring_uc_credit',
  );
  return $items;
}

/**
 * Credit card process and renew.
 */
function uc_recurring_uc_credit($order, &$fee, $op) {
  // Invoke the correct payment method.
  return uc_recurring_invoke($fee->fee_handler, $op, array($order, &$fee));
}
