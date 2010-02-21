<?php
// $Id$

/**
 * Implementation of hook_ca_entity().
 */
function uc_recurring_subscription_ca_entity() {
  $entities['uc_recurring_subscription'] = array(
    '#title' => t('Recurring subscription'),
    '#type' => 'object',
  );

  return $entities;
}

/**
 * Implementation of hook_ca_condition().
 */
function uc_recurring_subscription_ca_condition() {
  /*
  $conditions['uc_recurring_subscription_event_type'] = array(
    '#title' => t('Check the subscription event type'),
    '#category' => t('ClickandBuy'),
    '#callback' => 'uc_recurring_subscription_condition_event_type',
    '#arguments' => array(
      'event' => array('#entity' => 'uc_recurring_subscription'),
    ),
  );
*/
  return $conditions;
}

/**
 * Implementation of hook_ca_action().
 */
function uc_recurring_subscription_ca_action() {
  $actions['uc_recurring_subscription_grant_role'] = array(
    '#title' => t('Grant a role on an order.'),
    '#category' => t('Recurring'),
    '#callback' => 'uc_recurring_subscription_action_grant_role',
    '#arguments' => array(
      'order' => array(
        '#entity' => 'uc_order',
        '#title' => t('Order'),
      ),
    ),
  );

  $actions['uc_recurring_subscription_revoke_role'] = array(
    '#title' => t('Revoke a role on an order.'),
    '#category' => t('ClickandBuy'),
    '#callback' => 'uc_recurring_subscription_action_revoke_role',
    '#arguments' => array(
      'order' => array(
        '#entity' => 'uc_order',
        '#title' => t('Order'),
      ),
    ),
  );

  return $actions;
}

/**
 * Implementation of hook_ca_trigger().
 */
function uc_recurring_subscription_ca_trigger() {
  // use the existing uc_recurring triggers
  $triggers = array();
  return $triggers;
}

/**
 * Implementation of hook_ca_predicate().
 */
function uc_recurring_subscription_ca_predicate() {
  $predicates = array();

  // Set the order status to "Completed" when checkout is complete, none
  // of the products are shippable, and the balance is less than or equal to 0.
  $predicates['uc_recurring_subscription_grant_roles'] = array(
    '#title' => t('Update users roles on renewal processed'),
    '#trigger' => 'uc_recurring_renewal_complete',
    '#class' => 'recurring',
    '#status' => 1,
    '#weight' => 1,
    '#conditions' => array(
      '#operator' => 'AND',
      '#conditions' => array(
        array(
          '#name' => 'uc_payment_condition_order_balance',
          '#title' => t('If the balance is less than or equal to $0.00.'),
          '#argument_map' => array(
            'order' => 'order',
          ),
          '#settings' => array(
            'negate' => FALSE,
            'balance_comparison' => 'less_equal',
          ),
        ),
        array(
          '#name' => 'uc_order_condition_is_shippable',
          '#title' => t('If the order is not shippable.'),
          '#argument_map' => array(
            'order' => 'order',
          ),
          '#settings' => array(
            'negate' => TRUE,
          ),
        ),
      ),
    ),
    '#actions' => array(
      array(
        '#name' => 'uc_order_update_status',
        '#title' => t('Grant user roles.'),
        '#argument_map' => array(
          'order' => 'order',
        ),
        '#settings' => array(
          'order_status' => 'completed',
        ),
      )
    ),
  );

  return $predicates;
}



/**
 * Check a subscription event type.
 */
function uc_recurring_subscription_condition_event_type($event, $settings) {
  return in_array($event, array_filter($settings['event']));
}

/**
 *
 */
function uc_recurring_subscription_condition_event_type_form($form_state, $settings = array()) {
  $form['event'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Event types'),
    '#options' => array(
      'activated' => 'Subscription activated',
      'recurring' => 'Subscription renewed',
      'blocked' => 'Subscription blocked',
      'unblocked' => 'Subscription unblocked',
      //'ended' => 'Subscription ended',
      'terminated' => 'Subscription terminated',
    ),
    '#default_value' => is_array($settings['event']) ? $settings['event'] : array(),
  );

  return $form;
}

/**
 * Grant a role.
 */
function uc_recurring_subscription_action_grant_role($order, $subscription, $settings) {
  if ($settings 
  $rid = $settings['role'];
  $roles = user_roles(TRUE);

  if ($roles[$rid]) {
    $account = user_load($order->uid);
    $account->roles += array($rid => $roles[$rid]);
    user_save($account, array('roles' => $account->roles));
    watchdog('uc_recurring', 'Granted !role role to !user', array('!role' => $roles[$rid], '!user' => $account->name));
  }
}

function uc_recurring_subscription_action_grant_role_form($form_state, $settings = array()) {
  $form['role_option'] = array(
    '#type' => 'select',
    '#title' => 'Select role from',
    '#options' => array(
      'custom' => 'Custom Role',
      'subscription' => 'Subscription Settings',
    )
  );

  $roles = user_roles(TRUE);
  unset($roles[DRUPAL_AUTHENTICATED_RID]);
  if (empty($roles)) {
    $form['role_error'] = array(
      '#value' => t('There are no roles available to select. <a href="!url">Add a role</a>.', array('!url' => url('admin/user/roles'))),
    );
  }
  else { 
    $form['role'] = array(
      '#type' => 'select',
      '#title' => t('Role'),
      '#options' => $roles,
      '#default_value' => $settings['role'],
    );
  }
  return $form;
}

/**
 * Revoke a role.
 */
function uc_recurring_subscription_action_revoke_role($order, $settings) {
  $rid = $settings['role'];
  $roles = user_roles(TRUE);

  if ($roles[$rid]) {
    $account = user_load($order->uid);
    unset($account->roles[$rid]);
    user_save($account, array('roles' => $account->roles));
    watchdog('uc_recurring', 'Revoked !role role from !user', array('!role' => $roles[$rid], '!user' => $account->name));
 }
}

function uc_recurring_subscription_action_revoke_role_form($form_state, $settings = array()) {
  return uc_recurring_subscription_action_grant_role_form($form_state, $settings);
}
