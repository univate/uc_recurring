uc_recurring
~~~~~

uc_recurring is a drupal module to provide recurring billing to the ubercart project.

INSTALL
~~~~~~~

See the getting started guild on installing drupal modules:
http://drupal.org/getting-started/install-contrib/modules

USAGE
~~~~~

This module allows you to add recurring payments to an Product so that when a
order is placed with that product a payment schedule will be setup to charge
the user a set amount on a regular interval. 

Step 1: Enable module on your drupal site.

Step 2: Add recurring payments to a product:
  * Open product you want to create a recurring payment schedule on.
  * Click on "edit"
  * Click on "features"
  * Under "Add a new feature", select "Recurring fee"
  * Fill in the form as required
NOTE: you can add more then one recurring fee to a product and by selecting
different SKU's based on different attribute options setup different payment
options for the one product. e.g. weekly/monthly subscriptions.

Step 3: Setup Recurring Payments:
(This step can be skipped if you do not accept payments on site and you do not
have the uc_payment module enabled)
  * Requirement: Installed and setup payment gateways in ubercart.
  * Go to:
    "Store administration" -> "Configuration" -> "Products Settings" -> "Edit" -> "Product Features"
  * Select payment methods that should be allows to process recurring payments,
    only the methods selected will be shown on the checkout page when a order
    includes a recurring product.

You site should be ready to accept orders with recurring payments.

TESTING
~~~~~~~
Ubercart includes a test payment gateway called test_gateway. This gateway
emulates a credit card payment gateway and uc_recurring supports this gateway.

If you are attempting to test if uc_recurring is setup correctly this is a good
gateway to initally test against before setup up your own live gateway.

DEVELOPERS
~~~~~~~~~~
This modules includes the file uc_recurring.api.php which is an attempt to
define all the drupal hooks this module exposes to developers.

To integrate with a new payment gateway you should first look at the
hook_recurring_info() function as this defines all the details uc_recurring
needs to work with a new gateway.

LICENSE
~~~~~~~

This software licensed under the GNU General Public License 2.0.
