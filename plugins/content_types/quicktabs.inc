<?php

/**
 * @file
 * "No context" sample content type. It operates with no context at all. It would
 * be basically the same as a 'custom content' block, but it's not even that
 * sophisticated.
 *
 */

/**
 * Plugins are described by creating a $plugin array which will be used
 * by the system that includes this file.
 */
$plugin = array(
  'title' => t('Quicktabs'),
  'description' => t("Include Quicktabs' block and pass arguments into Views's tabs if applicable"),
  'content_types' => array('quicktabs_content_quicktabs_content_type_content_types'),
  'content type' => 'quicktabs_content_quicktabs_content_type_content_type',
  'defaults' => array('views'),
  'icon' => 'icon_example.png',
  'category' => array(t('Quicktabs'), -9),
  'all contexts' => TRUE
);

/**
 * Return all available Quicktabs.
 */
function quicktabs_content_quicktabs_content_type_content_types() {
  $types = array();
  $blocks = quicktabs_block();
  foreach ($blocks as $delta => $block){
    $types["quicktabs2-$delta"] = array(
      'title' => $block['info'],
      'category' => t('Quicktabs')
    );
  }
  return $types;
}

/**
 * Return Subtype info for a specific quicktab.
 * @param type $subtype
 * @return type
 */
function quicktabs_content_quicktabs_content_type_content_type($subtype) {
  $types = quicktabs_content_quicktabs_content_type_content_types();
  return $types[$subtype];
}


/**
 * Run-time rendering of the body of the block.
 *
 * @param $subtype
 * @param $conf
 *   Configuration as done at admin time.
 * @param $args
 * @param $context
 *   Context - in this case we don't have any.
 *
 * @return
 *   An object with at least title and content members.
 */
function quicktabs_content_quicktabs_content_type_render($subtype, $conf, $panel_args, $contexts) {
  $block = new stdClass();
  
  list($module, $delta) = explode('-', $subtype);
  $qt = quicktabs_load($delta);
  
  // The title actually used in rendering
  $block->title = check_plain($qt['title']);

  foreach($qt['tabs'] as &$tab) {
    if ($tab['type'] == 'view') {
      if (!empty($conf['views'][$tab['vid']])) {
        $tab['args'] = _quicktabs_content_quicktabs_content_type_views_args($conf['views'][$tab['vid']], $panel_args, $contexts);
      }
    }
  }
  
  $block->content = theme('quicktabs', $qt);
  return $block;
}

/**
 * 'Edit form' callback for the content type.
 */
function quicktabs_content_quicktabs_content_type_edit_form(&$form, &$form_state) {
  
  // Load up the views content_type plugin since we are going to lean on it.
  ctools_get_content_type('views');
  
  // Get existing config.
  $conf = $form_state['conf'];
  
  // Subtype format is quicktabs-$delta (block id).
  list($module, $delta) = explode('-', $form_state['subtype_name']);
  
  // Load up the quicktab.
  $qt = quicktabs_load($delta); 
  
  // Iterate over tabs and create settings for each View tab.
  foreach($qt['tabs'] as $key => $tab) {
    if ($tab['type'] == 'view') {
      
      $form[$tab['vid']] = array(
        '#type' => 'fieldset',
        '#collabsible' => TRUE,
        '#collapsed' => FALSE,
        '#title' => t('@tab: @view View Arguments',array('@tab' => $tab['title'], '@view' => $tab['vid'])),
        '#tree' => TRUE,
        'views_tab' => array(
          '#type' => 'value',
          '#value' => TRUE
        )
      );
      
      // Prepare form_states.
      $views_form = array();
      $views_conf = $conf['views'][$tab['vid']];
      
      // Flatten pager settings.
      if (is_array($views_conf['pager_settings'])) {
        foreach ($views_conf['pager_settings'] as $key => $value) {
          $views_conf[$key] = $value;
        }
      }  
      unset($views_conf['pager_settings']);
      
      $views_form_state = array(
        'conf' => $views_conf,
        'subtype_name' => $tab['vid'],
        'contexts' => $form_state['contexts']
      );
      
      // Merge our fieldset with views_content's form.
      $form[$tab['vid']] = array_merge($form[$tab['vid']], views_content_views_content_type_edit_form($views_form, $views_form_state));
    }
  }
  return $form;
}

function quicktabs_content_quicktabs_content_type_edit_form_submit(&$form, &$form_state) {
  
  // We only store the Views' fieldsets here.  Other settings will need new logic.
  $form_state['conf'] = array();
  $form_state['conf']['views'] = array();
  foreach ($form_state['values'] as $key => $value) {
    if (is_array($value) && !empty($value['views_tab'])) {
      $form_state['conf']['views'][$key] = $value;
    }
  }
  
}

/**
 * Prepares arguments to be passed into View per pane settings.
 * 
 * @param type $conf
 * @param type $panel_args
 * @param type $contexts
 * @return type 
 */
function _quicktabs_content_quicktabs_content_type_views_args($conf, $panel_args, $contexts) {

  // Stolen from the views content_type renderer.
  $arguments = explode('/', $_GET['q']);
  $args = $conf['args'];

  foreach ($arguments as $id => $arg) {
    $args = str_replace("%$id", $arg, $args);
  }

  foreach ($panel_args as $id => $arg) {
    if (is_string($arg)) {
      $args = str_replace("@$id", $arg, $args);
    }
  }

  $args = preg_replace(',/?(%\d|@\d),', '', $args);
  $args = $args ? explode('/', $args) : array();

  if ($conf['panel_args'] && is_array($panel_args)) {
    $args = array_merge($panel_args, $args);
  }

  if (isset($conf['context']) && is_array($conf['context'])) {
    foreach ($conf['context'] as $count => $context_info) {
      if (!strpos($context_info, '.')) {
        // old skool: support pre-converter contexts as well.
        $cid = $context_info;
        $converter = '';
      }
      else {
        list($cid, $converter) = explode('.', $context_info, 2);
      }
      if (!empty($contexts[$cid])) {
        $arg = ctools_context_convert_context($contexts[$cid], $converter);
        array_splice($args, $count, 0, array($arg));
      }
    }
  }
  
  return implode('/', $args);
}