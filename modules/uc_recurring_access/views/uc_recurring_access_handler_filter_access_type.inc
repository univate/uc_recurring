<?php
// $Id$

/**
 * @file
 * Views handler
 */

/**
 *
 */
class uc_recurring_access_handler_filter_access_type extends views_handler_filter_in_operator {

  function get_value_options() {
    if (!isset($this->value_options)) {
      $this->value_title = t('Access type');
      $this->value_options = _uc_recurring_access_rules();
    }
  }

}
