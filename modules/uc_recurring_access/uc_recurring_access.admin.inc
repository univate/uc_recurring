<?php
// $Id$

/**
 * @file
 * Admin function for recurring access.
 */

/**
 * Admin settings.
 */
function uc_recurring_access_admin_settings() {
  $types = node_get_types('types');
  foreach ($types as $type => $node_type) {
    $options[$type] = $node_type->name;
  }
  $form['uc_recurring_access_interval'] = array(
    '#type' => 'select',
    '#title' => t('Access applied in these intervals'),
    '#description' => t('Allows access to be feed to users at smaller intervals then their purchases (pick a number smaller then what you are after), this allows a user to pay for say a yearly subscription, but only get the content each day or week, rather then all at once. It requires that cron is setup and running at an interval smaller then the interval selected below.'),
    '#options' => array(
      '3600' => 'Hourly',
      '86400' => 'Daily',
      '604800' => 'Weekly',
      '2628000' => 'Monthly',
      '31536000' => 'Yearly',
    ),
    '#default_value' => variable_get('uc_recurring_access_interval', '31536000'),
  );
  $form['uc_recurring_access_content_types'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Select which content types to apply recurring access permissions on.'),
    '#options' => $options,
    '#default_value' => variable_get('uc_recurring_access_content_types', array()),
  );
  $form = system_settings_form($form);
  return $form;
}


