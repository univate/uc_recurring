<?php
// $Id: uc_views_handler_field_country.inc,v 1.1 2009/03/23 09:26:49 madsph Exp $

/**
 * @file
 * Views handler: Convert seconds into a Human friendly field
 */

/**
 * Return a time string.
 */
class uc_recurring_access_handler_field_time extends views_handler_field {
  function render($values) {
    return theme('uc_recurring_time', $values->{$this->field_alias});
  }
}
