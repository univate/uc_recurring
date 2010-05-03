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
  $form['uc_recurring_access_content_types'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Select which content types to apply recurring access permissions on.'),
    '#options' => $options,
    '#default_value' => variable_get('uc_recurring_access_content_types', array()),
  );
  $form = system_settings_form($form);
  return $form;
}


