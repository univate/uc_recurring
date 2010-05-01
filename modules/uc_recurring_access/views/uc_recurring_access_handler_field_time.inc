<?php
// $Id$

/**
 * @file
 * Views handler
 */

/**
 *
 */
class uc_recurring_access_handler_field_time extends views_handler_field {
  function render($values) {
    return theme('uc_recurring_time', $values->{$this->field_alias});
  }
}
