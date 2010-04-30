<?php
// $Id: uc_views_handler_field_country.inc,v 1.1 2009/03/23 09:26:49 madsph Exp $

/**
 * @file
 * Views handler: Convert seconds into a Human friendly field
 */

/**
 * Return a time string.
 */
class uc_recurring_access_handler_filter_access_type extends views_handler_filter_in_operator {

  function get_value_options() {
    if (!isset($this->value_options)) {
      $this->value_title = t('Access type');
      $this->value_options = _uc_recurring_access_rules();
    }
  }

}
